#include "pcma.h"
#include "jitter.h"
#include "pty.h"
#include "rtp.h"
#include "sip.h"
#include "v21.h"
#include "v22.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
static volatile sig_atomic_t stopping;
static void on_signal(int sig) { (void)sig; stopping=1; }

struct config {
    const char *bind_ip,*public_ip,*allowed_ip,*tty_path,*user_agent,*sdp_origin,*sdp_name;
    uint16_t sip_port,rtp_port; int speed;
};
static const char *env_or(const char *name,const char *fallback) { const char *v=getenv(name); return v&&*v?v:fallback; }
static uint16_t env_port(const char *name,uint16_t fallback) { long v=strtol(env_or(name,"0"),NULL,10); return v>0&&v<65536?(uint16_t)v:fallback; }
static int safe_text(const char *s) { return s && !strpbrk(s,"\r\n"); }
static uint64_t now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000; }
static int udp_bind(const char *ip,uint16_t port) {
    int fd=socket(AF_INET,SOCK_DGRAM,0); if(fd<0)return -1; int one=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one);
    struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(port)};
    if(inet_pton(AF_INET,ip,&a.sin_addr)!=1 || bind(fd,(struct sockaddr*)&a,sizeof a)<0){close(fd);return -1;} return fd;
}
static int source_allowed(const struct config *c,const struct sockaddr_in *from) {
    if(!c->allowed_ip[0]) return 1;
    char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET,&from->sin_addr,ip,sizeof ip);
    char list[512]; snprintf(list,sizeof list,"%s",c->allowed_ip); char *save=NULL;
    for(char *p=strtok_r(list,",",&save);p;p=strtok_r(NULL,",",&save)){while(*p==' ')p++;if(!strcmp(p,ip))return 1;} return 0;
}
static void send_sip(int fd,const struct sockaddr_in *to,const char *data,int length) {
    if(length>0) (void)sendto(fd,data,(size_t)length,0,(const struct sockaddr*)to,sizeof *to);
}

int main(void) {
    struct config c={
        .bind_ip=env_or("SOFTMODEM_BIND_IP","0.0.0.0"), .public_ip=env_or("SOFTMODEM_PUBLIC_IP","127.0.0.1"),
        .allowed_ip=env_or("SOFTMODEM_ALLOWED_IPS",""), .tty_path=env_or("SOFTMODEM_TTY","/tmp/ttySOFTMODEM0"),
        .user_agent=env_or("SOFTMODEM_USER_AGENT","SIP-Softmodem/0.1"), .sdp_origin=env_or("SOFTMODEM_SDP_ORIGIN","softmodem"),
        .sdp_name=env_or("SOFTMODEM_SDP_NAME","SIP Softmodem"), .sip_port=env_port("SOFTMODEM_SIP_PORT",5060),
        .rtp_port=env_port("SOFTMODEM_RTP_PORT",10000), .speed=atoi(env_or("SOFTMODEM_SPEED","1200"))};
    if(!safe_text(c.user_agent)||!safe_text(c.sdp_origin)||!safe_text(c.sdp_name)){fprintf(stderr,"invalid CR/LF in identity setting\n");return 2;}
    if(c.speed!=300&&c.speed!=1200){fprintf(stderr,"SOFTMODEM_SPEED must be 300 or 1200\n");return 2;}
    int sip_fd=udp_bind(c.bind_ip,c.sip_port),rtp_fd=udp_bind(c.bind_ip,c.rtp_port); char slave[256]; int pty_fd=pty_open_link(c.tty_path,slave,sizeof slave);
    if(sip_fd<0||rtp_fd<0||pty_fd<0){perror("startup");return 1;} fcntl(pty_fd,F_SETFL,fcntl(pty_fd,F_GETFL)|O_NONBLOCK);
    signal(SIGINT,on_signal);signal(SIGTERM,on_signal); srand((unsigned)(time(NULL)^getpid()));
    fprintf(stderr,"SIP %s:%u, RTP %s:%u, %d bit/s, PTY %s -> %s\n",c.bind_ip,c.sip_port,c.bind_ip,c.rtp_port,c.speed,c.tty_path,slave);

    struct sockaddr_in peer_rtp={0},peer_sip={0}; socklen_t peer_sip_len=sizeof peer_sip; int call=0,acked=0;
    char dialog_id[256]="", tag[32],last_ok[4096]=""; int last_ok_len=0; snprintf(tag,sizeof tag,"%08x",(unsigned)rand());
    struct v21 modem; struct v22 modem22; v21_init(&modem);v22_init(&modem22); struct rtp_sender tx={(uint16_t)rand(),(uint32_t)rand(),(uint32_t)rand()};
    struct jitter jitter; jitter_reset(&jitter); uint64_t next_tx=now_ms(),call_started=0,last_rtp=0; uint64_t media_samples=0;
    while(!stopping) {
        struct pollfd fds[]={{sip_fd,POLLIN,0},{rtp_fd,POLLIN,0},{pty_fd,POLLIN,0}};
        int timeout=call?(int)(next_tx>now_ms()?next_tx-now_ms():0):-1;
        int ready=poll(fds,ARRAY_SIZE(fds),timeout); if(ready<0){if(errno==EINTR)continue;perror("poll");break;}
        if(fds[0].revents&POLLIN) {
            char input[8193],output[4096],sdp_body[1024],remote_ip[64]; struct sockaddr_in from; socklen_t fl=sizeof from;
            ssize_t n=recvfrom(sip_fd,input,sizeof input-1,0,(struct sockaddr*)&from,&fl); if(n<=0)continue; input[n]='\0';
            struct sip_request req; if(sip_parse(input,&req)<0)continue;
            char contact[256]; snprintf(contact,sizeof contact,"sip:modem@%s:%u",c.public_ip,c.sip_port);
            if(!source_allowed(&c,&from)) { int m=sip_make_response(output,sizeof output,&req,403,"Forbidden",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue; }
            if(!strcmp(req.method,"OPTIONS")) {int m=sip_make_response(output,sizeof output,&req,200,"OK",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue;}
            if(!strcmp(req.method,"INVITE")) {
                if(call && !strcmp(req.call_id,dialog_id)) { send_sip(sip_fd,&from,last_ok,last_ok_len); continue; }
                uint16_t remote_port; if(call || sip_pcma_endpoint(req.body,remote_ip,sizeof remote_ip,&remote_port)<0) {
                    int code=call?486:488;int m=sip_make_response(output,sizeof output,&req,code,call?"Busy Here":"Not Acceptable Here",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue;
                }
                peer_rtp.sin_family=AF_INET;peer_rtp.sin_port=htons(remote_port);if(inet_pton(AF_INET,remote_ip,&peer_rtp.sin_addr)!=1)continue;
                peer_sip=from;peer_sip_len=fl;snprintf(dialog_id,sizeof dialog_id,"%s",req.call_id);
                sip_make_sdp(sdp_body,sizeof sdp_body,c.public_ip,c.rtp_port,c.sdp_origin,c.sdp_name);
                int m=sip_make_response(output,sizeof output,&req,200,"OK",tag,contact,c.user_agent,sdp_body);send_sip(sip_fd,&from,output,m);
                memcpy(last_ok,output,(size_t)m);last_ok_len=m;call=1;acked=0;media_samples=0;call_started=now_ms();last_rtp=0;next_tx=call_started;jitter_reset(&jitter);v21_init(&modem);v22_init(&modem22);fprintf(stderr,"call from %s, RTP %s:%u at %d bit/s\n",inet_ntoa(from.sin_addr),remote_ip,remote_port,c.speed);
            } else if(!strcmp(req.method,"ACK") && call && !strcmp(req.call_id,dialog_id)) acked=1;
            else if(!strcmp(req.method,"BYE") && call && !strcmp(req.call_id,dialog_id)) {int m=sip_make_response(output,sizeof output,&req,200,"OK",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);call=acked=0;fprintf(stderr,"call ended\n");}
        }
        if(call && (fds[1].revents&POLLIN)) {
            uint8_t packet[2048];ssize_t n=recv(rtp_fd,packet,sizeof packet,0);struct rtp_packet rp;
            if(n>0&&rtp_parse(packet,(size_t)n,&rp)==0&&rp.payload_type==8&&rp.payload_len==160){jitter_put(&jitter,rp.sequence,rp.payload,rp.payload_len);last_rtp=now_ms();}
        }
        if(call && acked && (fds[2].revents&POLLIN)) {uint8_t bytes[512];ssize_t n=read(pty_fd,bytes,sizeof bytes);if(n>0){if(c.speed==1200)v22_write(&modem22,bytes,(size_t)n);else v21_write(&modem,bytes,(size_t)n);}}
        uint64_t now=now_ms();
        while(call && now>=next_tx) {
            int16_t pcm[160]; uint8_t alaw[160],packet[172];
            uint8_t inbound[160];if(jitter_get(&jitter,inbound,sizeof inbound)>0){int16_t rx[160];pcma_decode_buffer(inbound,rx,160);uint8_t bytes[256];size_t got;if(c.speed==1200){v22_receive(&modem22,rx,160);got=v22_read(&modem22,bytes,sizeof bytes);}else{v21_receive(&modem,rx,160);got=v21_read(&modem,bytes,sizeof bytes);}if(got)(void)write(pty_fd,bytes,got);}
            if(media_samples<24000) for(size_t i=0;i<160;i++){double phase=2.0*M_PI*2100.0*(media_samples+i)/8000.0;pcm[i]=(int16_t)(sin(phase)*10000.0);}
            else if(media_samples<24600) memset(pcm,0,sizeof pcm); else if(c.speed==1200)v22_generate(&modem22,pcm,160);else v21_generate(&modem,pcm,160);
            pcma_encode_buffer(pcm,alaw,160);size_t length=rtp_build(&tx,alaw,160,packet,sizeof packet);
            sendto(rtp_fd,packet,length,0,(struct sockaddr*)&peer_rtp,sizeof peer_rtp);media_samples+=160;next_tx+=20;now=now_ms();
        }
        now=now_ms();if(call&&((!acked&&now-call_started>32000)||(last_rtp&&now-last_rtp>30000))){fprintf(stderr,"call timed out\n");call=acked=0;last_ok_len=0;}
    }
    (void)peer_sip;(void)peer_sip_len;close(pty_fd);close(rtp_fd);close(sip_fd);unlink(c.tty_path);return 0;
}
