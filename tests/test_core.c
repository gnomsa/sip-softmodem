#include "pcma.h"
#include "at.h"
#include "jitter.h"
#include "rtp.h"
#include "sip.h"
#include "v21.h"
#include "v22.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_pcma(void) {
    const int16_t values[]={-30000,-1000,0,1000,30000};
    for(size_t i=0;i<sizeof values/sizeof values[0];i++) {
        int decoded=pcma_decode(pcma_encode(values[i]));
        assert(abs(decoded-values[i]) < 1200);
    }
}
static void test_at(void){struct at_modem a;at_init(&a);a.max_speed=33600;char out[512]={0};const uint8_t cmd[]="ATE0V1S0=2\r";assert(at_feed(&a,cmd,sizeof cmd-1,out,sizeof out)==AT_EVENT_NONE);assert(!a.echo&&a.verbose&&a.s0==2&&strstr(out,"OK"));memset(out,0,sizeof out);assert(at_ring(&a,out,sizeof out)==AT_EVENT_NONE);assert(strstr(out,"RING"));assert(at_ring(&a,out,sizeof out)==AT_EVENT_ANSWER);const uint8_t answer[]="ATA\r";assert(at_feed(&a,answer,sizeof answer-1,out,sizeof out)==AT_EVENT_ANSWER);const uint8_t dial[]="ATDP+42012345\r";assert(at_feed(&a,dial,sizeof dial-1,out,sizeof out)==AT_EVENT_DIAL);assert(a.pulse_dial&&!strcmp(a.dial_number,"+42012345"));const uint8_t redial[]="ATDL\r";assert(at_feed(&a,redial,sizeof redial-1,out,sizeof out)==AT_EVENT_DIAL);assert(!strcmp(a.dial_number,"+42012345"));const uint8_t mode[]="AT+MS=V34\r";assert(at_feed(&a,mode,sizeof mode-1,out,sizeof out)==AT_EVENT_NONE);assert(a.preferred_speed==33600);}
static void test_rtp(void) {
    struct rtp_sender s={42,8000,99};uint8_t payload[160]={1,2,3},wire[172];
    size_t n=rtp_build(&s,payload,sizeof payload,wire,sizeof wire);struct rtp_packet p;
    assert(n==172&&rtp_parse(wire,n,&p)==0);assert(p.payload_type==8&&p.sequence==42&&p.timestamp==8000&&p.payload_len==160);
}
static void test_jitter(void) {
    struct jitter j;jitter_reset(&j);uint8_t in[160],out[160];
    for(int i=0;i<12;i++){memset(in,i,sizeof in);assert(jitter_put(&j,(uint16_t)(100+i),in,sizeof in)==1);}
    assert(jitter_get(&j,out,sizeof out)==160&&out[0]==0);
    assert(jitter_get(&j,out,sizeof out)==160&&out[0]==1);
    struct jitter loss;jitter_reset(&loss);
    for(int i=0;i<12;i++){if(i==3)continue;memset(in,i,sizeof in);assert(jitter_put(&loss,(uint16_t)(200+i),in,sizeof in)==1);}
    for(int i=0;i<3;i++)assert(jitter_get(&loss,out,sizeof out)==160&&out[0]==i);
    assert(jitter_get(&loss,out,sizeof out)==0);
    assert(jitter_get(&loss,out,sizeof out)==0);
    assert(jitter_get(&loss,out,sizeof out)==-1);
    assert(jitter_get(&loss,out,sizeof out)==160&&out[0]==4);
}
static void test_sip(void) {
    char invite[]="INVITE sip:m@host SIP/2.0\r\nVia: SIP/2.0/UDP 10.0.0.1:5060;branch=z\r\nFrom: <sip:a@x>;tag=a\r\nTo: <sip:m@host>\r\nCall-ID: test\r\nCSeq: 1 INVITE\r\nContent-Type: application/sdp\r\nContent-Length: 57\r\n\r\nv=0\r\nc=IN IP4 10.0.0.1\r\nm=audio 4000 RTP/AVP 8 0\r\n";
    struct sip_request r;assert(sip_parse(invite,&r)==0);char ip[64];uint16_t port;assert(sip_pcma_endpoint(r.body,ip,sizeof ip,&port)==0);assert(!strcmp(ip,"10.0.0.1")&&port==4000);
    char sdp[512];assert(sip_make_sdp(sdp,sizeof sdp,"10.0.0.2",10000,"alice","Lab Modem")>0);assert(strstr(sdp,"o=alice ")&&strstr(sdp,"s=Lab Modem"));
    char response[2048];assert(sip_make_response(response,sizeof response,&r,200,"OK","tag","sip:m@10.0.0.2","Custom-UA",sdp)>0);assert(strstr(response,"Server: Custom-UA"));
    char request[2048];assert(sip_make_uac_request(request,sizeof request,"INVITE","sip:123@10.0.0.1","SIP/2.0/UDP 10.0.0.2:5060;branch=z","<sip:m@10.0.0.2>;tag=f","<sip:123@10.0.0.1>","out-call",1,"sip:m@10.0.0.2","UA",sdp)>0);assert(strstr(request,"INVITE sip:123@10.0.0.1 SIP/2.0"));
    char wire[]="SIP/2.0 486 Busy Here\r\nVia: SIP/2.0/UDP x;branch=z\r\nFrom: <sip:a@x>;tag=a\r\nTo: <sip:b@y>;tag=b\r\nCall-ID: out\r\nCSeq: 1 INVITE\r\nContent-Length: 0\r\n\r\n";struct sip_response sr;assert(sip_parse_response(wire,&sr)==0&&sr.status==486&&!strcmp(sr.call_id,"out"));
    char trying[]="SIP/2.0 100 Connecting\r\nVia: SIP/2.0/UDP x;branch=z\r\nFrom: <sip:a@x>;tag=a\r\nTo: <sip:b@y>\r\nCall-ID: out\r\nCSeq: 1 INVITE\r\nContent-Length: 0\r\n\r\n";
    assert(sip_parse_response(trying,&sr)==0&&sr.status==100);
    assert(sip_invite_should_retransmit(0));
    assert(!sip_invite_should_retransmit(100));
    assert(!sip_invite_should_retransmit(183));
    assert(!sip_invite_should_retransmit(200));
}
static void test_v21_receive(void) {
    struct v21 modem; v21_init(&modem);
    const unsigned byte=0x41;
    int bits[14]={1,1,1,0};
    for(int i=0;i<8;i++) bits[4+i]=(byte>>i)&1;
    bits[12]=1; bits[13]=1;
    int16_t samples[400]; size_t used=0; double phase=0.0,clock=0.0;int index=0;
    while(index<14 && used<400) {
        double frequency=bits[index]?980.0:1180.0;
        phase += 2.0*3.14159265358979323846*frequency/8000.0;
        samples[used++]=(int16_t)(sin(phase)*12000.0);
        clock+=1.0;if(clock>=8000.0/300.0){clock-=8000.0/300.0;index++;}
    }
    v21_receive(&modem,samples,used);uint8_t out=0;
    assert(v21_read(&modem,&out,1)==1);assert(out==byte);
}
static void test_v22_transmit(void) {
    struct v22 modem;v22_init(&modem);uint8_t text[]={0x55,0xaa};assert(v22_write(&modem,text,sizeof text)==sizeof text);
    int16_t samples[160];v22_generate(&modem,samples,160);long long energy=0;for(size_t i=0;i<160;i++)energy+=(long long)samples[i]*samples[i];assert(energy>1000000000LL);
}
int main(void) { test_at();test_pcma();test_rtp();test_jitter();test_sip();test_v21_receive();test_v22_transmit();puts("all core tests passed");return 0; }
