#include "v22.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define QMASK 4095u

void v22_init(struct v22 *v){memset(v,0,sizeof *v);v->tx_phase=0.0;}
size_t v22_write(struct v22 *v,const uint8_t*d,size_t n){size_t z=0;while(z<n&&((v->tx_tail+1)&QMASK)!=v->tx_head){v->tx_queue[v->tx_tail]=d[z++];v->tx_tail=(v->tx_tail+1)&QMASK;}return z;}
size_t v22_read(struct v22 *v,uint8_t*d,size_t n){size_t z=0;while(z<n&&v->rx_head!=v->rx_tail){d[z++]=v->rx_queue[v->rx_head];v->rx_head=(v->rx_head+1)&QMASK;}return z;}
static int raw_bit(struct v22*v){if(!v->tx_bits){if(v->tx_head==v->tx_tail)return 1;unsigned b=v->tx_queue[v->tx_head];v->tx_head=(v->tx_head+1)&QMASK;v->tx_frame=(1u<<9)|(b<<1);v->tx_bits=10;}return(v->tx_frame>>(10-v->tx_bits--))&1u;}
static int scramble(struct v22*v,int bit){int out=bit^((v->tx_scrambler>>13)&1u)^((v->tx_scrambler>>16)&1u);if(out){if(++v->tx_ones>=64){out^=1;v->tx_ones=0;}}else v->tx_ones=0;v->tx_scrambler=((v->tx_scrambler<<1)|(unsigned)out)&0x1ffffu;return out;}
static double phase_change(int a,int b){int dibit=(a<<1)|b;switch(dibit){case 0:return M_PI/2;case 1:return 0;case 3:return 3*M_PI/2;default:return M_PI;}}
void v22_generate(struct v22*v,int16_t*out,size_t n){for(size_t i=0;i<n;i++){if(v->tx_clock<=0){int a,b;/* Answer handshake: unscrambled marks, then scrambled marks, then user stream. */if(v->tx_samples<7200){a=b=1;}else if(v->tx_samples<15200){a=scramble(v,1);b=scramble(v,1);}else{a=scramble(v,raw_bit(v));b=scramble(v,raw_bit(v));}v->tx_phase+=phase_change(a,b);v->tx_clock+=8000.0/600.0;}double carrier=v->tx_phase+2*M_PI*2400.0*(double)v->tx_samples/8000.0;out[i]=(int16_t)(sin(carrier)*10000.0);v->tx_samples++;v->tx_clock-=1.0;}}
static int descramble(struct v22*v,int bit){int out=bit^((v->rx_descrambler>>13)&1u)^((v->rx_descrambler>>16)&1u);v->rx_descrambler=((v->rx_descrambler<<1)|(unsigned)bit)&0x1ffffu;return out;}
static void uart_bit(struct v22*v,int bit){if(!v->rx_receiving){if(!bit){v->rx_receiving=1;v->rx_bits=0;v->rx_frame=0;}return;}if(v->rx_bits<8)v->rx_frame|=(unsigned)bit<<v->rx_bits++;else{if(bit){size_t next=(v->rx_tail+1)&QMASK;if(next!=v->rx_head){v->rx_queue[v->rx_tail]=(uint8_t)v->rx_frame;v->rx_tail=next;}}v->rx_receiving=0;}}
static void symbol(struct v22*v){double phase=atan2(v->rx_q,v->rx_i);if(v->rx_have_phase){double d=phase-v->rx_previous_phase;while(d<0)d+=2*M_PI;while(d>=2*M_PI)d-=2*M_PI;int a,b;if(d<M_PI/4||d>=7*M_PI/4){a=0;b=1;}else if(d<3*M_PI/4){a=0;b=0;}else if(d<5*M_PI/4){a=1;b=0;}else{a=1;b=1;}a=descramble(v,a);b=descramble(v,b);if(v->rx_samples>=15200){uart_bit(v,a);uart_bit(v,b);}}v->rx_previous_phase=phase;v->rx_have_phase=1;v->rx_i=v->rx_q=0;}
void v22_receive(struct v22*v,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double x=in[k]/32768.0;v->rx_i+=x*cos(v->rx_phase);v->rx_q-=x*sin(v->rx_phase);v->rx_phase+=2*M_PI*1200.0/8000.0;if(v->rx_phase>=2*M_PI)v->rx_phase-=2*M_PI;v->rx_clock+=1;v->rx_samples++;if(v->rx_clock>=8000.0/600.0){symbol(v);v->rx_clock-=8000.0/600.0;}}}
