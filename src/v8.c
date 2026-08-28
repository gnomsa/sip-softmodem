#include "v8.h"
#include <string.h>

static size_t put_octet(uint8_t*out,size_t cap,size_t at,uint8_t value){
    if(at<cap)out[at]=0;
    at++;
    for(unsigned b=0;b<8;b++){if(at<cap)out[at]=(value>>b)&1u;at++;}
    if(at<cap)out[at]=1;
    return at+1;
}
static uint8_t get_octet(const uint8_t*bits,size_t at){uint8_t v=0;for(unsigned b=0;b<8;b++)v|=(bits[at+1+b]&1u)<<b;return v;}
size_t v8_encode_menu(const struct v8_menu*m,uint8_t*out,size_t cap){
    size_t at=0;for(int i=0;i<10;i++){if(at<cap)out[at]=1;at++;}
    static const uint8_t sync[10]={0,0,0,0,0,0,1,1,1,1};
    for(int i=0;i<10;i++){if(at<cap)out[at]=sync[i];at++;}
    at=put_octet(out,cap,at,0xc1); /* call function: unspecified V-series data */
    uint8_t mod0=0x05;if(m->modes&V8_MODE_V34)mod0|=0x40;
    uint8_t mod1=0x10;if(m->modes&V8_MODE_V32)mod1|=0x01;if(m->modes&V8_MODE_V22)mod1|=0x02;
    uint8_t mod2=0x10;if(m->modes&V8_MODE_V21)mod2|=0x80;
    at=put_octet(out,cap,at,mod0);at=put_octet(out,cap,at,mod1);at=put_octet(out,cap,at,mod2);
    if(m->lapm)at=put_octet(out,cap,at,0x2a); /* protocol tag plus LAPM code 100 */
    if(m->cellular||m->digital_access){uint8_t access=0x0d;if(m->cellular)access|=0x20;if(m->digital_access)access|=0x80;at=put_octet(out,cap,at,access);}
    return at;
}
int v8_decode_menu(const uint8_t*b,size_t n,struct v8_menu*m){
    if(!b||!m||n<60)return -1;
    for(size_t i=0;i<10;i++)if(!b[i])return -1;
    static const uint8_t sync[10]={0,0,0,0,0,0,1,1,1,1};for(size_t i=0;i<10;i++)if((b[10+i]&1u)!=sync[i])return -1;
    memset(m,0,sizeof *m);int have_call=0,have_mod=0,in_mod=0,mod_ext=0;size_t at=20;
    while(at+10<=n){if(b[at]||!b[at+9])return -1;uint8_t o=get_octet(b,at);at+=10;
        if(!(o&0x10)){
            unsigned tag=o&0x0f;in_mod=0;mod_ext=0;
            if(tag==0x01){have_call=1;if((o&0xe0)!=0xc0)return -1;}
            else if(tag==0x05){have_mod=in_mod=1;if(o&0x40)m->modes|=V8_MODE_V34;}
            else if(tag==0x0a){m->lapm=(o&0xe0)==0x20;}
            else if(tag==0x0d){m->cellular=(o&0x60)!=0;m->digital_access=(o&0x80)!=0;}
        }else if(in_mod&&((o&0x38)==0x10)){
            if(mod_ext++==0){if(o&1)m->modes|=V8_MODE_V32;if(o&2)m->modes|=V8_MODE_V22;}
            else if(o&0x80)m->modes|=V8_MODE_V21;
        }
    }
    return have_call&&have_mod?0:-1;
}
unsigned v8_joint_modes(unsigned a,unsigned b){return a&b;}
unsigned v8_select_mode(unsigned m){if((m&V8_MODE_V90_ANALOG)&&(m&V8_MODE_V90_DIGITAL))return V8_MODE_V90_ANALOG|V8_MODE_V90_DIGITAL;if(m&V8_MODE_V34)return V8_MODE_V34;if(m&V8_MODE_V32)return V8_MODE_V32;if(m&V8_MODE_V22)return V8_MODE_V22;if(m&V8_MODE_V21)return V8_MODE_V21;return 0;}
size_t v8_encode_cj(uint8_t*out,size_t cap){size_t at=0;for(int i=0;i<3;i++)at=put_octet(out,cap,at,0);return at;}
