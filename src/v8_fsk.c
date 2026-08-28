#include "v8_fsk.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RMASK 4095u
void v8_fsk_init(struct v8_fsk*f,int high){memset(f,0,sizeof *f);f->high_channel=high!=0;}
size_t v8_fsk_set_sequence(struct v8_fsk*f,const uint8_t*b,size_t n){if(n>sizeof f->tx)n=sizeof f->tx;memcpy(f->tx,b,n);f->tx_count=n;f->tx_at=0;return n;}
void v8_fsk_generate(struct v8_fsk*f,int16_t*out,size_t n){for(size_t i=0;i<n;i++){if(f->tx_clock<=0){f->tx_clock+=8000.0/300.0;if(f->tx_count)f->tx_at=(f->tx_at+1)%f->tx_count;}int bit=f->tx_count?f->tx[(f->tx_at+f->tx_count-1)%f->tx_count]:1;double hz=f->high_channel?(bit?1650.0:1850.0):(bit?980.0:1180.0);f->tx_phase+=2*M_PI*hz/8000.0;if(f->tx_phase>=2*M_PI)f->tx_phase-=2*M_PI;out[i]=(int16_t)(sin(f->tx_phase)*10000);f->tx_clock-=1;}}
void v8_fsk_receive(struct v8_fsk*f,const int16_t*in,size_t n){double mark=f->high_channel?1650.0:980.0,space=f->high_channel?1850.0:1180.0;for(size_t i=0;i<n;i++){double x=in[i]/32768.0,pm=f->rx_phase*mark,ps=f->rx_phase*space;f->mi+=x*cos(pm);f->mq+=x*sin(pm);f->si+=x*cos(ps);f->sq+=x*sin(ps);f->rx_phase+=2*M_PI/8000.0;f->rx_clock++;if(f->rx_clock>=8000.0/300.0){double me=f->mi*f->mi+f->mq*f->mq,se=f->si*f->si+f->sq*f->sq;size_t next=(f->rx_tail+1)&RMASK;if(next!=f->rx_head){f->rx[f->rx_tail]=me>se;f->rx_tail=next;}f->mi=f->mq=f->si=f->sq=0;f->rx_clock-=8000.0/300.0;}}}
size_t v8_fsk_read(struct v8_fsk*f,uint8_t*b,size_t n){size_t z=0;while(z<n&&f->rx_head!=f->rx_tail){b[z++]=f->rx[f->rx_head];f->rx_head=(f->rx_head+1)&RMASK;}return z;}
