#include "v32.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define QM 4095u
void v32_init(struct v32*v,int rate){memset(v,0,sizeof*v);v->rate=rate;v->bits=rate>=9600?4:2;}
size_t v32_write(struct v32*v,const uint8_t*d,size_t n){size_t z=0;while(z<n&&((v->tt+1)&QM)!=v->th){v->tq[v->tt]=d[z++];v->tt=(v->tt+1)&QM;}return z;}
size_t v32_read(struct v32*v,uint8_t*d,size_t n){size_t z=0;while(z<n&&v->rh!=v->rt){d[z++]=v->rq[v->rh];v->rh=(v->rh+1)&QM;}return z;}
static int bit(struct v32*v){if(!v->tb){if(v->th==v->tt)return 1;unsigned b=v->tq[v->th];v->th=(v->th+1)&QM;v->tf=(1u<<9)|(b<<1);v->tb=10;}return(v->tf>>(10-v->tb--))&1;}
static int take(struct v32*v){int b=0;for(int i=0;i<v->bits;i++)b=(b<<1)|bit(v);return b;}
static void levels(struct v32*v,int code,double*i,double*q){if(v->bits==2){*i=(code&1)?-1:1;*q=(code&2)?-1:1;}else{static const double l[4]={-3,-1,3,1};*i=l[(code>>2)&3];*q=l[code&3];}}
static void uart(struct v32*v,int b){if(!v->rx_receiving){if(!b){v->rx_receiving=1;v->rx_bits=0;v->rframe=0;}return;}if(v->rx_bits<8)v->rframe|=(unsigned)b<<v->rx_bits++;else{if(b){size_t z=(v->rt+1)&QM;if(z!=v->rh){v->rq[v->rt]=(uint8_t)v->rframe;v->rt=z;}}v->rx_receiving=0;}}
void v32_generate(struct v32*v,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(v->tx_clock<=0){double i,q;levels(v,take(v),&i,&q);double norm=v->bits==2?9000.0:3200.0;v->carrier_i=i*norm;v->carrier_q=q*norm;v->tx_clock+=8000.0/2400.0;}double p=2*M_PI*1800.0*(double)v->tx_samples/8000.0;out[k]=(int16_t)(v->carrier_i*cos(p)-v->carrier_q*sin(p));v->tx_samples++;v->tx_clock-=1;}}
void v32_receive(struct v32*v,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double p=2*M_PI*1800.0*(double)v->rx_samples/8000.0,x=in[k];v->carrier_i+=x*cos(p);v->carrier_q-=x*sin(p);v->rx_samples++;v->rx_clock++;if(v->rx_clock>=8000.0/2400.0){int code;if(v->bits==2)code=(v->carrier_i<0?1:0)|(v->carrier_q<0?2:0);else{int a=v->carrier_i<0?2:0;if(fabs(v->carrier_i)<2000)a|=1;int b=v->carrier_q<0?2:0;if(fabs(v->carrier_q)<2000)b|=1;code=(a<<2)|b;}for(int j=v->bits-1;j>=0;j--)uart(v,(code>>j)&1);v->carrier_i=v->carrier_q=0;v->rx_clock-=8000.0/2400.0;}}}
