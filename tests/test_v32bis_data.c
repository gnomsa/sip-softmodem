#include "v32bis_data.h"
#include "v32bis_map.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void run(int rate){struct v32bis_data a,b;assert(v32bis_data_init(&a,V32_STD_CALL,rate)==0);assert(v32bis_data_init(&b,V32_STD_ANSWER,rate)==0);uint8_t source[256],got[300]={0};for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*37u+11u);assert(v32bis_data_write(&a,source,sizeof source)==sizeof source);size_t have=0;for(unsigned n=0;n<3000&&have<sizeof source;n++){unsigned label=v32bis_data_next(&a);struct v32bis_point p;assert(v32bis_map_point(rate,label,&p)==0);double noise=(int)(n%7)-3;assert(v32bis_data_put(&b,p.i+noise*.03,p.q-noise*.02)>=0);have+=v32bis_data_read(&b,got+have,sizeof got-have);}assert(have>=sizeof source&&!memcmp(source,got,sizeof source));}

static void deleted_stops(void)
{
    static const uint8_t source[]={0x7e,0xff,0x03,0xc0,0x21,0x7d,0x5e};
    int bits[1024];size_t count=0;
    for(size_t byte=0;byte<sizeof source;byte++){
        bits[count++]=0;
        for(unsigned bit=0;bit<8;bit++)
            bits[count++]=(source[byte]>>bit)&1;
        /* Delete every inter-character stop bit as permitted by V.14. */
        if(byte+1==sizeof source)bits[count++]=1;
    }
    while(count<600)bits[count++]=1;

    struct v32_std_scrambler scr;
    struct v32bis_trellis trellis;
    struct v32bis_data rx;
    v32_std_scrambler_init(&scr,V32_STD_CALL);
    v32bis_trellis_init(&trellis);
    assert(v32bis_data_init(&rx,V32_STD_ANSWER,9600)==0);
    unsigned previous=0;
    for(size_t at=0;at<count;){
        unsigned packed=0;
        for(unsigned bit=0;bit<4;bit++){
            int input=at<count?bits[at++]:1;
            packed=(packed<<1)|(unsigned)v32_std_scramble(&scr,input);
        }
        unsigned y12=v32bis_trellis_diff_encode(packed>>2,previous);
        previous=y12;
        unsigned label=(v32bis_trellis_encode(&trellis,y12)<<2)|(packed&3u);
        struct v32bis_point point;
        assert(v32bis_map_point(9600,label,&point)==0);
        assert(v32bis_data_put(&rx,point.i,point.q)>=0);
    }
    uint8_t got[sizeof source];
    assert(v32bis_data_read(&rx,got,sizeof got)==sizeof got);
    assert(!memcmp(got,source,sizeof source));
}

int main(void){run(7200);run(9600);run(12000);run(14400);deleted_stops();struct v32bis_data d;assert(v32bis_data_init(&d,V32_STD_CALL,4800)<0);puts("V.32bis data codec: exact bytes and V.14 deleted stop bits pass");return 0;}
