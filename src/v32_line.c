#include "v32_line.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MASK 4095u
void v32_line_init(struct v32_line*l){memset(l,0,sizeof *l);}
size_t v32_line_write(struct v32_line*l,const enum v32_carrier_state*s,size_t n){size_t z=0;while(z<n&&((l->tt+1)&MASK)!=l->th){l->tx[l->tt]=(uint8_t)s[z++];l->tt=(l->tt+1)&MASK;}return z;}
size_t v32_line_read(struct v32_line*l,enum v32_carrier_state*s,size_t n){size_t z=0;while(z<n&&l->rh!=l->rt){s[z++]=(enum v32_carrier_state)l->rx[l->rh];l->rh=(l->rh+1)&MASK;}return z;}
static void point(enum v32_carrier_state s,double*i,double*q){static const double xy[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};*i=xy[s&3][0]*8500;*q=xy[s&3][1]*8500;}
void v32_line_generate(struct v32_line*l,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(l->tx_clock<=0){enum v32_carrier_state s=V32_STATE_A;if(l->th!=l->tt){s=(enum v32_carrier_state)l->tx[l->th];l->th=(l->th+1)&MASK;}point(s,&l->tx_i,&l->tx_q);l->tx_clock+=8000.0/2400.0;}double p=2*M_PI*1800.0*(double)l->tx_samples/8000.0;out[k]=(int16_t)(l->tx_i*cos(p)-l->tx_q*sin(p));l->tx_samples++;l->tx_clock-=1;}}
static enum v32_carrier_state decide(double i,double q){if(q>=0)return i>=0?V32_STATE_A:V32_STATE_B;return i<0?V32_STATE_C:V32_STATE_D;}
void v32_line_receive(struct v32_line*l,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double p=2*M_PI*1800.0*(double)l->rx_samples/8000.0,c=cos(p),s=sin(p),x=in[k];l->rx_i+=x*c;l->rx_q-=x*s;l->rx_cc+=c*c;l->rx_ss+=s*s;l->rx_cs+=c*s;l->rx_samples++;l->rx_clock++;if(l->rx_clock>=8000.0/2400.0){double det=l->rx_cc*l->rx_ss-l->rx_cs*l->rx_cs,i=l->rx_i,q=l->rx_q;if(fabs(det)>1e-9){i=(l->rx_ss*l->rx_i+l->rx_cs*l->rx_q)/det;q=(l->rx_cs*l->rx_i+l->rx_cc*l->rx_q)/det;}size_t next=(l->rt+1)&MASK;if(next!=l->rh){l->rx[l->rt]=(uint8_t)decide(i,q);l->rt=next;}l->rx_i=l->rx_q=l->rx_cc=l->rx_ss=l->rx_cs=0;l->rx_clock-=8000.0/2400.0;}}}
