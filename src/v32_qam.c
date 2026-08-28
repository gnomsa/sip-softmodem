#include "v32_qam.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MASK 4095u
void v32_qam_init(struct v32_qam*q){memset(q,0,sizeof *q);}
size_t v32_qam_write(struct v32_qam*q,const uint8_t*s,size_t n){size_t z=0;while(z<n&&((q->tt+1)&MASK)!=q->th){q->tx[q->tt]=s[z++]&15;q->tt=(q->tt+1)&MASK;}return z;}
size_t v32_qam_read(struct v32_qam*q,uint8_t*s,size_t n){size_t z=0;while(z<n&&q->rh!=q->rt){s[z++]=q->rx[q->rh];q->rh=(q->rh+1)&MASK;}return z;}
static void point(unsigned code,double*i,double*q){unsigned y=code>>2,q3=(code>>1)&1,q4=code&1;double xmag=q4?3:1,ymag=q3?3:1;*i=(y&2?xmag:-xmag)*3000;*q=(y&1?ymag:-ymag)*3000;}
void v32_qam_generate(struct v32_qam*q,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(q->tx_clock<=0){unsigned code=0;if(q->th!=q->tt){code=q->tx[q->th];q->th=(q->th+1)&MASK;}point(code,&q->tx_i,&q->tx_q);q->tx_clock+=8000.0/2400.0;}double p=2*M_PI*1800.0*(double)q->tx_samples/8000.0;out[k]=(int16_t)(q->tx_i*cos(p)-q->tx_q*sin(p));q->tx_samples++;q->tx_clock-=1;}}
static uint8_t decide(double i,double q){unsigned y=(i>=0?2:0)|(q>=0?1:0),q3=fabs(q)>6000,q4=fabs(i)>6000;return(uint8_t)((y<<2)|(q3<<1)|q4);}
void v32_qam_receive(struct v32_qam*q,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double p=2*M_PI*1800.0*(double)q->rx_samples/8000.0,c=cos(p),s=sin(p),x=in[k];q->rx_i+=x*c;q->rx_q-=x*s;q->rx_cc+=c*c;q->rx_ss+=s*s;q->rx_cs+=c*s;q->rx_samples++;q->rx_clock++;if(q->rx_clock>=8000.0/2400.0){double det=q->rx_cc*q->rx_ss-q->rx_cs*q->rx_cs,i=q->rx_i,v=q->rx_q;if(fabs(det)>1e-9){i=(q->rx_ss*q->rx_i+q->rx_cs*q->rx_q)/det;v=(q->rx_cs*q->rx_i+q->rx_cc*q->rx_q)/det;}size_t next=(q->rt+1)&MASK;if(next!=q->rh){q->rx[q->rt]=decide(i,v);q->rt=next;}q->rx_i=q->rx_q=q->rx_cc=q->rx_ss=q->rx_cs=0;q->rx_clock-=8000.0/2400.0;}}}
