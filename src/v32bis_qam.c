#include "v32bis_qam.h"
#include "v32bis_map.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MASK 4095u
static unsigned labels_for_rate(int rate){return rate==7200?16:rate==9600?32:rate==12000?64:rate==14400?128:0;}
int v32bis_qam_init(struct v32bis_qam*q,int rate){unsigned labels=labels_for_rate(rate);if(!q||!labels)return-1;memset(q,0,sizeof *q);q->rate=rate;double energy=0;for(unsigned n=0;n<labels;n++){struct v32bis_point p;if(v32bis_map_point(rate,n,&p)<0)return-1;energy+=p.i*p.i+p.q*p.q;}q->gain=9000.0/sqrt(energy/labels);return 0;}
size_t v32bis_qam_write(struct v32bis_qam*q,const uint8_t*s,size_t n){size_t z=0;unsigned labels=labels_for_rate(q->rate);while(z<n&&((q->tt+1)&MASK)!=q->th){if(s[z]>=labels)break;q->tx[q->tt]=s[z++];q->tt=(q->tt+1)&MASK;}return z;}
size_t v32bis_qam_read(struct v32bis_qam*q,struct v32bis_sample*s,size_t n){size_t z=0;while(z<n&&q->rh!=q->rt){s[z++]=q->rx[q->rh];q->rh=(q->rh+1)&MASK;}return z;}
void v32bis_qam_generate(struct v32bis_qam*q,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(q->tx_clock<=0){struct v32bis_point p={0,0};if(q->th!=q->tt){(void)v32bis_map_point(q->rate,q->tx[q->th],&p);q->th=(q->th+1)&MASK;}q->tx_i=p.i*q->gain;q->tx_q=p.q*q->gain;q->tx_clock+=8000.0/2400.0;}double phase=2*M_PI*1800.0*q->tx_samples/8000.0;double v=q->tx_i*cos(phase)-q->tx_q*sin(phase);if(v>32767)v=32767;if(v<-32768)v=-32768;out[k]=(int16_t)v;q->tx_samples++;q->tx_clock-=1;}}
void v32bis_qam_receive(struct v32bis_qam*q,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double phase=2*M_PI*1800.0*q->rx_samples/8000.0,c=cos(phase),s=sin(phase),x=in[k];q->rx_i+=x*c;q->rx_q-=x*s;q->rx_cc+=c*c;q->rx_ss+=s*s;q->rx_cs+=c*s;q->rx_samples++;q->rx_clock++;if(q->rx_clock>=8000.0/2400.0){double det=q->rx_cc*q->rx_ss-q->rx_cs*q->rx_cs,i=q->rx_i,v=q->rx_q;if(fabs(det)>1e-9){i=(q->rx_ss*q->rx_i+q->rx_cs*q->rx_q)/det;v=(q->rx_cs*q->rx_i+q->rx_cc*q->rx_q)/det;}size_t next=(q->rt+1)&MASK;if(next!=q->rh){q->rx[q->rt]=(struct v32bis_sample){i/q->gain,v/q->gain};q->rt=next;}q->rx_i=q->rx_q=q->rx_cc=q->rx_ss=q->rx_cs=0;q->rx_clock-=8000.0/2400.0;}}}
