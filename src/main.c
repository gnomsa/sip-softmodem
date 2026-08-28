#include "pcma.h"
#include "at.h"
#include "jitter.h"
#include "pty.h"
#include "rtp.h"
#include "sip.h"
#include "v21.h"
#include "v22.h"
#include "v22bis.h"
#include "v32.h"
#include "v32_session.h"
#include "tone_detector.h"
#include "ansam.h"
#include "v8.h"
#include "v8_fsk.h"
#include "v8_session.h"
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
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
static volatile sig_atomic_t stopping;
static void on_signal(int sig) { (void)sig; stopping=1; }

struct config {
    const char *bind_ip,*public_ip,*allowed_ip,*tty_path,*user_agent,*sdp_origin,*sdp_name,*outbound_host,*protocols;
    uint16_t sip_port,rtp_port,outbound_port; int speed,max_rate,v8;
};
static const char *env_or(const char *name,const char *fallback) { const char *v=getenv(name); return v&&*v?v:fallback; }
static uint16_t env_port(const char *name,uint16_t fallback) { long v=strtol(env_or(name,"0"),NULL,10); return v>0&&v<65536?(uint16_t)v:fallback; }
static int safe_text(const char *s) { return s && !strpbrk(s,"\r\n"); }
static int token_enabled(const char*list,const char*name){if(!strcasecmp(list,"ALL"))return 1;char copy[256];snprintf(copy,sizeof copy,"%s",list);char*save=NULL;for(char*p=strtok_r(copy,",",&save);p;p=strtok_r(NULL,",",&save)){while(*p==' ')p++;if(!strcasecmp(p,name))return 1;}return 0;}
static int select_speed(const char*protocols,int max_rate){if(token_enabled(protocols,"V32")&&max_rate>=9600)return 9600;if(token_enabled(protocols,"V32")&&max_rate>=4800)return 4800;if(token_enabled(protocols,"V22BIS")&&max_rate>=2400)return 2400;if(token_enabled(protocols,"V22")&&max_rate>=1200)return 1200;if(token_enabled(protocols,"V21")&&max_rate>=300)return 300;if(strcasecmp(protocols,"ALL")&&token_enabled(protocols,"EXPERIMENTAL_QAM")&&max_rate>=9600)return 9600;if(strcasecmp(protocols,"ALL")&&token_enabled(protocols,"EXPERIMENTAL_QAM")&&max_rate>=4800)return 4800;return 0;}
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
        .rtp_port=env_port("SOFTMODEM_RTP_PORT",10000), .outbound_host=env_or("SOFTMODEM_OUTBOUND_HOST",""),
        .outbound_port=env_port("SOFTMODEM_OUTBOUND_PORT",5060), .protocols=env_or("SOFTMODEM_PROTOCOLS","ALL"),
        .max_rate=atoi(env_or("SOFTMODEM_MAX_RATE","33600")), .v8=atoi(env_or("SOFTMODEM_V8","1")), .speed=0};
    if(!safe_text(c.user_agent)||!safe_text(c.sdp_origin)||!safe_text(c.sdp_name)){fprintf(stderr,"invalid CR/LF in identity setting\n");return 2;}
    c.speed=select_speed(c.protocols,c.max_rate);if(!c.speed){fprintf(stderr,"no implemented protocol enabled (V21,V22,V22BIS,V32; optional EXPERIMENTAL_QAM)\n");return 2;}
    int standard_v32=token_enabled(c.protocols,"V32")&&c.speed>=4800;
    int sip_fd=udp_bind(c.bind_ip,c.sip_port),rtp_fd=udp_bind(c.bind_ip,c.rtp_port); char slave[256]; int pty_fd=pty_open_link(c.tty_path,slave,sizeof slave);
    if(sip_fd<0||rtp_fd<0||pty_fd<0){perror("startup");return 1;} fcntl(pty_fd,F_SETFL,fcntl(pty_fd,F_GETFL)|O_NONBLOCK);
    signal(SIGINT,on_signal);signal(SIGTERM,on_signal); srand((unsigned)(time(NULL)^getpid()));
    fprintf(stderr,"SIP %s:%u, RTP %s:%u, %d bit/s, PTY %s -> %s\n",c.bind_ip,c.sip_port,c.bind_ip,c.rtp_port,c.speed,c.tty_path,slave);

    struct sockaddr_in peer_rtp={0},peer_sip={0},out_peer={0}; socklen_t peer_sip_len=sizeof peer_sip; int call=0,acked=0,pending=0,answer_requested=0,connect_reported=0,dialing=0,dial_requested=0,answer_side=0,ans_observed=0,ans_complete=0,v8_done=0,modem_started=0,v8_last_state=-1;
    struct sip_request pending_req;char remote_ip[64]="";uint16_t remote_port=0;
    char dialog_id[256]="", tag[32],last_ok[4096]="",last_ring[2048]=""; int last_ok_len=0,last_ring_len=0; snprintf(tag,sizeof tag,"%08x",(unsigned)rand());
    char out_uri[256]="",out_invite[4096]="",out_via[256]="",out_from[256]="",out_contact[256]="";int out_len=0;
    struct v21 modem; struct v22 modem22; struct v22bis modem22bis; struct v32 modem32; struct v32_session modem32std; v21_init(&modem);v22_init(&modem22);v22bis_init(&modem22bis);v32_init(&modem32,c.speed>=9600?9600:4800);v32_session_init(&modem32std,V32_STD_CALL,1,c.speed>=9600); struct rtp_sender tx={(uint16_t)rand(),(uint32_t)rand(),(uint32_t)rand()};
    struct tone_detector ans_detector;tone_detector_init(&ans_detector,2100.0,160);
    struct ansam_generator ansam_tx;struct ansam_detector ansam_rx;struct v8_fsk v8tx,v8rx;struct v8_session v8s;struct v8_menu v8local={.modes=V8_MODE_V21|V8_MODE_V22|(standard_v32?V8_MODE_V32:0)};uint8_t v8_menu_bits[128],v8_rx_bits[4096];size_t v8_rx_count=0;uint64_t v8_state_started=0;
    struct at_modem at;at_init(&at);at.s0=1;at.max_speed=c.speed;
    struct jitter jitter; jitter_reset(&jitter); uint64_t next_tx=now_ms(),call_started=0,last_rtp=0,next_ring=0,dial_started=0,next_invite=0; uint64_t media_samples=0;
    while(!stopping) {
        struct pollfd fds[]={{sip_fd,POLLIN,0},{rtp_fd,POLLIN,0},{pty_fd,POLLIN,0}};
        uint64_t before_poll=now_ms();
        int timeout=call?(int)(next_tx>before_poll?next_tx-before_poll:0):pending?(int)(next_ring>before_poll?next_ring-before_poll:0):dialing?(int)(next_invite>before_poll?next_invite-before_poll:0):-1;
        int ready=poll(fds,ARRAY_SIZE(fds),timeout); if(ready<0){if(errno==EINTR)continue;perror("poll");break;}
        if(fds[0].revents&POLLIN) {
            char input[8193],output[4096]; struct sockaddr_in from; socklen_t fl=sizeof from;
            ssize_t n=recvfrom(sip_fd,input,sizeof input-1,0,(struct sockaddr*)&from,&fl); if(n<=0)continue; input[n]='\0';
            if(!strncmp(input,"SIP/2.0 ",8)) {
                struct sip_response sr;
                if(!dialing || sip_parse_response(input,&sr)<0 || strcmp(sr.call_id,dialog_id)) continue;
                if(sr.status==486 || sr.status==600) { char b[64]; size_t z=at_busy(&at,b,sizeof b); (void)write(pty_fd,b,z); dialing=0; continue; }
                if(sr.status>=400) { char b[64]; size_t z=at_no_carrier(&at,b,sizeof b); (void)write(pty_fd,b,z); dialing=0; continue; }
                if(sr.status>=200 && sr.status<300) {
                    char rip[64]; uint16_t rport;
                    if(sip_pcma_endpoint(sr.body,rip,sizeof rip,&rport)<0) continue;
                    peer_rtp.sin_family=AF_INET; peer_rtp.sin_port=htons(rport); if(inet_pton(AF_INET,rip,&peer_rtp.sin_addr)!=1) continue;
                    char ack[2048]; int z=sip_make_uac_request(ack,sizeof ack,"ACK",out_uri,out_via,out_from,sr.to,dialog_id,1,out_contact,c.user_agent,"");
                    send_sip(sip_fd,&out_peer,ack,z); dialing=0; call=acked=1; answer_side=0;ans_observed=ans_complete=0;tone_detector_init(&ans_detector,2100.0,160);ansam_generator_init(&ansam_tx);ansam_detector_init(&ansam_rx);v8_fsk_init(&v8tx,0);v8_fsk_init(&v8rx,1);v8_session_init(&v8s,V8_CALL_DCE,&v8local);v8_rx_count=0;v8_last_state=-1;v8_done=!c.v8;modem_started=!c.v8;connect_reported=0; media_samples=0; call_started=now_ms(); next_tx=call_started; last_rtp=0; jitter_reset(&jitter); v21_init(&modem); v22_init(&modem22); v22bis_init(&modem22bis); v32_init(&modem32,c.speed>=9600?9600:4800);v32_session_init(&modem32std,V32_STD_CALL,1,c.speed>=9600); v21_set_answer_role(&modem,0); v22_set_answer_role(&modem22,0); v22bis_set_answer_role(&modem22bis,0);if(c.speed==2400&&!c.v8)v22bis_start_handshake(&modem22bis,0);
                }
                continue;
            }
            struct sip_request req; if(sip_parse(input,&req)<0)continue;
            char contact[256]; snprintf(contact,sizeof contact,"sip:modem@%s:%u",c.public_ip,c.sip_port);
            if(!source_allowed(&c,&from)) { int m=sip_make_response(output,sizeof output,&req,403,"Forbidden",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue; }
            if(!strcmp(req.method,"OPTIONS")) {int m=sip_make_response(output,sizeof output,&req,200,"OK",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue;}
            if(!strcmp(req.method,"INVITE")) {
                if(call && !strcmp(req.call_id,dialog_id)) { send_sip(sip_fd,&from,last_ok,last_ok_len); continue; }
                if(pending && !strcmp(req.call_id,dialog_id)){send_sip(sip_fd,&from,last_ring,last_ring_len);continue;}
                if(call||pending || sip_pcma_endpoint(req.body,remote_ip,sizeof remote_ip,&remote_port)<0) {
                    int code=(call||pending)?486:488;int m=sip_make_response(output,sizeof output,&req,code,(call||pending)?"Busy Here":"Not Acceptable Here",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);continue;
                }
                peer_rtp.sin_family=AF_INET;peer_rtp.sin_port=htons(remote_port);if(inet_pton(AF_INET,remote_ip,&peer_rtp.sin_addr)!=1)continue;
                peer_sip=from;peer_sip_len=fl;pending_req=req;pending_req.body=NULL;snprintf(dialog_id,sizeof dialog_id,"%s",req.call_id);
                int m=sip_make_response(output,sizeof output,&req,180,"Ringing",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);memcpy(last_ring,output,(size_t)m);last_ring_len=m;
                pending=1;next_ring=now_ms()+6000;char atout[1024];enum at_event ev=at_ring_caller(&at,req.from,atout,sizeof atout);(void)write(pty_fd,atout,strnlen(atout,sizeof atout));if(ev==AT_EVENT_ANSWER)answer_requested=1;
            } else if(!strcmp(req.method,"ACK") && call && !strcmp(req.call_id,dialog_id)) acked=1;
            else if(!strcmp(req.method,"BYE") && call && !strcmp(req.call_id,dialog_id)) {int m=sip_make_response(output,sizeof output,&req,200,"OK",tag,contact,c.user_agent,"");send_sip(sip_fd,&from,output,m);call=acked=0;char atout[128];size_t z=at_no_carrier(&at,atout,sizeof atout);(void)write(pty_fd,atout,z);fprintf(stderr,"call ended\n");}
        }
        if(call && (fds[1].revents&POLLIN)) {
            uint8_t packet[2048];ssize_t n=recv(rtp_fd,packet,sizeof packet,0);struct rtp_packet rp;
            if(n>0&&rtp_parse(packet,(size_t)n,&rp)==0&&rp.payload_type==8&&rp.payload_len==160){jitter_put(&jitter,rp.sequence,rp.payload,rp.payload_len);last_rtp=now_ms();}
        }
        if(fds[2].revents&POLLIN) {uint8_t bytes[512];ssize_t n=read(pty_fd,bytes,sizeof bytes);if(n>0){if(at.online&&call&&acked){if(standard_v32)v32_session_write(&modem32std,bytes,(size_t)n);else if(c.speed>=4800)v32_write(&modem32,bytes,(size_t)n);else if(c.speed==2400)v22bis_write(&modem22bis,bytes,(size_t)n);else if(c.speed==1200)v22_write(&modem22,bytes,(size_t)n);else v21_write(&modem,bytes,(size_t)n);}else{char atout[1024];memset(atout,0,sizeof atout);enum at_event ev=at_feed(&at,bytes,(size_t)n,atout,sizeof atout);size_t z=strnlen(atout,sizeof atout);if(z)(void)write(pty_fd,atout,z);if(ev==AT_EVENT_ANSWER&&pending)answer_requested=1;if(ev==AT_EVENT_DIAL){if(!c.outbound_host[0]){char status[64];size_t q=at_no_dialtone(&at,status,sizeof status);(void)write(pty_fd,status,q);}else dial_requested=1;}if(ev==AT_EVENT_HANGUP){pending=0;dialing=0;call=acked=0;}}}}
        if(dial_requested&&!call&&!pending&&!dialing){dial_requested=0;out_peer.sin_family=AF_INET;out_peer.sin_port=htons(c.outbound_port);if(inet_pton(AF_INET,c.outbound_host,&out_peer.sin_addr)!=1){char b[64];size_t z=at_no_dialtone(&at,b,sizeof b);(void)write(pty_fd,b,z);}else{char body[1024];snprintf(dialog_id,sizeof dialog_id,"%08x@%s",(unsigned)rand(),c.public_ip);snprintf(out_uri,sizeof out_uri,"sip:%s@%s:%u",at.dial_number,c.outbound_host,c.outbound_port);snprintf(out_via,sizeof out_via,"SIP/2.0/UDP %s:%u;branch=z9hG4bK%08x;rport",c.public_ip,c.sip_port,(unsigned)rand());snprintf(out_from,sizeof out_from,"<sip:modem@%s>;tag=%s",c.public_ip,tag);snprintf(out_contact,sizeof out_contact,"sip:modem@%s:%u",c.public_ip,c.sip_port);sip_make_sdp(body,sizeof body,c.public_ip,c.rtp_port,c.sdp_origin,c.sdp_name);out_len=sip_make_uac_request(out_invite,sizeof out_invite,"INVITE",out_uri,out_via,out_from,out_uri,dialog_id,1,out_contact,c.user_agent,body);send_sip(sip_fd,&out_peer,out_invite,out_len);dialing=1;dial_started=now_ms();next_invite=dial_started+500;}}
        if(dialing&&now_ms()>=next_invite){send_sip(sip_fd,&out_peer,out_invite,out_len);next_invite=now_ms()+1000;}
        if(pending&&now_ms()>=next_ring){char atout[1024]={0};enum at_event ev=at_ring_caller(&at,pending_req.from,atout,sizeof atout);(void)write(pty_fd,atout,strnlen(atout,sizeof atout));if(ev==AT_EVENT_ANSWER)answer_requested=1;next_ring+=6000;}
        if(pending&&answer_requested){
            char output[4096],sdp_body[1024],contact[256];
            snprintf(contact,sizeof contact,"sip:modem@%s:%u",c.public_ip,c.sip_port);
            sip_make_sdp(sdp_body,sizeof sdp_body,c.public_ip,c.rtp_port,c.sdp_origin,c.sdp_name);
            int m=sip_make_response(output,sizeof output,&pending_req,200,"OK",tag,contact,c.user_agent,sdp_body);
            send_sip(sip_fd,&peer_sip,output,m);memcpy(last_ok,output,(size_t)m);last_ok_len=m;
            pending=0;answer_requested=0;call=1;acked=0;answer_side=1;ans_observed=ans_complete=0;
            tone_detector_init(&ans_detector,2100.0,160);ansam_generator_init(&ansam_tx);ansam_detector_init(&ansam_rx);
            v8_fsk_init(&v8tx,1);v8_fsk_init(&v8rx,0);v8_session_init(&v8s,V8_ANSWER_DCE,&v8local);
            v8_rx_count=0;v8_last_state=-1;v8_done=!c.v8;modem_started=!c.v8;connect_reported=0;media_samples=0;
            call_started=now_ms();last_rtp=0;next_tx=call_started;jitter_reset(&jitter);
            v21_init(&modem);v22_init(&modem22);v22bis_init(&modem22bis);v32_init(&modem32,c.speed>=9600?9600:4800);v32_session_init(&modem32std,V32_STD_ANSWER,1,c.speed>=9600);
            if(c.speed==2400&&!c.v8)v22bis_start_handshake(&modem22bis,1);
            fprintf(stderr,"answering RTP %s:%u at %d bit/s%s\n",remote_ip,remote_port,c.speed,c.v8?" with V.8":"");
        }
        uint64_t now=now_ms();
        while(call && now>=next_tx) {
            int16_t pcm[160]; uint8_t alaw[160],packet[172];
            if(c.v8&&!v8_done)v8_session_advance(&v8s,20);
            if(modem_started&&c.speed==2400)v22bis_advance(&modem22bis,20);
            uint8_t inbound[160];int jitter_result=jitter_get(&jitter,inbound,sizeof inbound);
            if(jitter_result<0&&standard_v32){v32_session_media_gap(&modem32std);fprintf(stderr,"RTP gap: requesting V.32 retrain\n");}
            if(jitter_result>0){
                int16_t rx[160];pcma_decode_buffer(inbound,rx,160);
                if(c.v8&&!v8_done){
                    if(!answer_side&&v8s.state==V8_WAIT){
                        int was=tone_detector_present(&ans_detector);tone_detector_process(&ans_detector,rx,160);
                        int is=tone_detector_present(&ans_detector);if(is)ans_observed=1;
                        ansam_detect(&ansam_rx,rx,160);if(ansam_present(&ansam_rx))v8_session_ansam(&v8s);
                        if(ans_observed&&was&&!is&&!ansam_present(&ansam_rx)){v8_done=1;modem_started=1;ans_complete=1;if(c.speed==2400)v22bis_start_handshake(&modem22bis,0);}
                    }
                    if(!v8_done){
                        v8_fsk_receive(&v8rx,rx,160);uint8_t rawbits[128];size_t nr=v8_fsk_read(&v8rx,rawbits,sizeof rawbits);
                        if(nr&&v8_rx_count+nr<=sizeof v8_rx_bits){memcpy(v8_rx_bits+v8_rx_count,rawbits,nr);v8_rx_count+=nr;}
                        for(size_t off=0;off+60<=v8_rx_count;off++){
                            struct v8_menu menu;
                            if(v8_decode_menu(v8_rx_bits+off,60,&menu)==0){v8_session_menu(&v8s,&menu);off+=59;}
                        }
                        if(answer_side&&v8s.state==V8_JM){
                            for(size_t off=0;off+30<=v8_rx_count;off++){int cj=1;for(size_t j=0;j<30;j++){int want=(j%10)==9;if(v8_rx_bits[off+j]!=(uint8_t)want){cj=0;break;}}if(cj){v8_session_cj(&v8s);break;}}
                        }
                    }
                }else if(modem_started){
                    uint8_t bytes[256];size_t got;
                    if(standard_v32){v32_session_receive(&modem32std,rx,160);got=v32_session_read(&modem32std,bytes,sizeof bytes);}
                    else if(c.speed>=4800){v32_receive(&modem32,rx,160);got=v32_read(&modem32,bytes,sizeof bytes);}
                    else if(c.speed==2400){v22bis_receive(&modem22bis,rx,160);got=v22bis_read(&modem22bis,bytes,sizeof bytes);}
                    else if(c.speed==1200){v22_receive(&modem22,rx,160);got=v22_read(&modem22,bytes,sizeof bytes);}
                    else{v21_receive(&modem,rx,160);got=v21_read(&modem,bytes,sizeof bytes);}
                    if(got&&at.online)(void)write(pty_fd,bytes,got);
                }
            }
            if(c.v8&&!v8_done&&(int)v8s.state!=v8_last_state){
                v8_last_state=v8s.state;v8_state_started=now_ms();size_t bits=0;
                if(v8s.state==V8_CM)bits=v8_encode_menu(&v8local,v8_menu_bits,sizeof v8_menu_bits);
                else if(v8s.state==V8_JM)bits=v8_encode_menu(&v8s.joint,v8_menu_bits,sizeof v8_menu_bits);
                else if(v8s.state==V8_CJ)bits=v8_encode_cj(v8_menu_bits,sizeof v8_menu_bits);
                if(bits)v8_fsk_set_sequence(&v8tx,v8_menu_bits,bits);
            }
            if(c.v8&&!v8_done&&!answer_side&&v8s.state==V8_CJ&&now_ms()-v8_state_started>=100)v8_session_cj(&v8s);
            if(c.v8&&!v8_done&&(v8s.state==V8_SELECTED||v8s.state==V8_FAILED)){
                v8_done=1;modem_started=1;ans_complete=1;
                if(c.speed==2400){v22bis_start_handshake(&modem22bis,answer_side);if(answer_side)v22bis_answer_sequence_complete(&modem22bis);}
                fprintf(stderr,"V.8 %s, starting modem family at %d bit/s\n",v8s.state==V8_SELECTED?"selected":"fallback",c.speed);
            }
            if(c.v8&&!v8_done){
                if(answer_side&&v8s.state==V8_ANSAM)ansam_generate(&ansam_tx,pcm,160);
                else if(v8s.state==V8_CM||v8s.state==V8_JM||v8s.state==V8_CJ)v8_fsk_generate(&v8tx,pcm,160);
                else memset(pcm,0,sizeof pcm);
            }else if(!c.v8&&answer_side&&media_samples<20800)for(size_t i=0;i<160;i++){double phase=2.0*M_PI*2100.0*(media_samples+i)/8000.0;pcm[i]=(int16_t)(sin(phase)*10000.0);}
            else if(!c.v8&&answer_side&&media_samples<21400)memset(pcm,0,sizeof pcm);
            else if(!modem_started||(!answer_side&&!ans_complete))memset(pcm,0,sizeof pcm);
            else if(standard_v32)v32_session_generate(&modem32std,pcm,160);
            else if(c.speed>=4800)v32_generate(&modem32,pcm,160);
            else if(c.speed==2400){if(answer_side)v22bis_answer_sequence_complete(&modem22bis);v22bis_generate(&modem22bis,pcm,160);}
            else if(c.speed==1200)v22_generate(&modem22,pcm,160);else v21_generate(&modem,pcm,160);
            pcma_encode_buffer(pcm,alaw,160);size_t length=rtp_build(&tx,alaw,160,packet,sizeof packet);
            sendto(rtp_fd,packet,length,0,(struct sockaddr*)&peer_rtp,sizeof peer_rtp);media_samples+=160;next_tx+=20;now=now_ms();
        }
        now=now_ms();if(call&&((!acked&&now-call_started>32000)||(last_rtp&&now-last_rtp>30000))){fprintf(stderr,"call timed out\n");call=acked=0;last_ok_len=0;}
        int negotiated=c.speed==2400?v22bis_selected_rate(&modem22bis):standard_v32?v32_session_rate(&modem32std):c.speed;
        int trained=modem_started&&(c.speed==2400?v22bis_connected(&modem22bis):standard_v32?v32_session_connected(&modem32std):((answer_side||ans_complete)&&now-call_started>5200));
        if(call&&acked&&!connect_reported&&trained){char atout[128];size_t z=at_connected(&at,negotiated,atout,sizeof atout);(void)write(pty_fd,atout,z);connect_reported=1;}
    }
    (void)peer_sip;(void)peer_sip_len;close(pty_fd);close(rtp_fd);close(sip_fd);unlink(c.tty_path);return 0;
}
