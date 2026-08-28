#include "pcma.h"
#include "rtp.h"
#include "sip.h"
#include "v21.h"
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
static void test_rtp(void) {
    struct rtp_sender s={42,8000,99};uint8_t payload[160]={1,2,3},wire[172];
    size_t n=rtp_build(&s,payload,sizeof payload,wire,sizeof wire);struct rtp_packet p;
    assert(n==172&&rtp_parse(wire,n,&p)==0);assert(p.payload_type==8&&p.sequence==42&&p.timestamp==8000&&p.payload_len==160);
}
static void test_sip(void) {
    char invite[]="INVITE sip:m@host SIP/2.0\r\nVia: SIP/2.0/UDP 10.0.0.1:5060;branch=z\r\nFrom: <sip:a@x>;tag=a\r\nTo: <sip:m@host>\r\nCall-ID: test\r\nCSeq: 1 INVITE\r\nContent-Type: application/sdp\r\nContent-Length: 57\r\n\r\nv=0\r\nc=IN IP4 10.0.0.1\r\nm=audio 4000 RTP/AVP 8 0\r\n";
    struct sip_request r;assert(sip_parse(invite,&r)==0);char ip[64];uint16_t port;assert(sip_pcma_endpoint(r.body,ip,sizeof ip,&port)==0);assert(!strcmp(ip,"10.0.0.1")&&port==4000);
    char sdp[512];assert(sip_make_sdp(sdp,sizeof sdp,"10.0.0.2",10000,"alice","Lab Modem")>0);assert(strstr(sdp,"o=alice ")&&strstr(sdp,"s=Lab Modem"));
    char response[2048];assert(sip_make_response(response,sizeof response,&r,200,"OK","tag","sip:m@10.0.0.2","Custom-UA",sdp)>0);assert(strstr(response,"Server: Custom-UA"));
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
int main(void) { test_pcma();test_rtp();test_sip();test_v21_receive();puts("all core tests passed");return 0; }
