#include "v32_line.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MASK 4095u
void v32_line_init(struct v32_line*l){memset(l,0,sizeof *l);}
void v32_line_set_pulse_shaped(struct v32_line*l,int enabled){l->pulse_shaped=enabled!=0;}
size_t v32_line_write(struct v32_line*l,const enum v32_carrier_state*s,size_t n){size_t z=0;while(z<n&&((l->tt+1)&MASK)!=l->th){l->tx[l->tt]=(uint8_t)s[z++];l->tt=(l->tt+1)&MASK;}return z;}
size_t v32_line_read(struct v32_line*l,enum v32_carrier_state*s,size_t n){size_t z=0;while(z<n&&l->rh!=l->rt){s[z++]=(enum v32_carrier_state)l->rx[l->rh];l->rh=(l->rh+1)&MASK;}return z;}
static void point(enum v32_carrier_state s,double*i,double*q){static const double xy[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};*i=xy[s&3][0]*8500;*q=xy[s&3][1]*8500;}
static double rrc(double t)
{
    const double beta=0.25;
    if(fabs(t)<1e-9)return 1.0+beta*(4.0/M_PI-1.0);
    if(fabs(fabs(t)-1.0/(4.0*beta))<1e-7)
        return beta/sqrt(2.0)*((1.0+2.0/M_PI)*sin(M_PI/(4.0*beta))+
               (1.0-2.0/M_PI)*cos(M_PI/(4.0*beta)));
    return (sin(M_PI*t*(1.0-beta))+4.0*beta*t*cos(M_PI*t*(1.0+beta)))/
           (M_PI*t*(1.0-(4.0*beta*t)*(4.0*beta*t)));
}
static int16_t shaped_sample(struct v32_line*l)
{
    double u=(double)l->tx_samples*2400.0/8000.0;
    uint64_t required=(uint64_t)floor(u)+1;
    while(l->shaped_loaded<required){
        enum v32_carrier_state state=V32_STATE_A;double i,q;
        if(l->th!=l->tt){state=(enum v32_carrier_state)l->tx[l->th];l->th=(l->th+1)&MASK;}
        point(state,&i,&q);unsigned slot=(unsigned)(l->shaped_loaded&31u);
        l->shaped_i[slot]=i;l->shaped_q[slot]=q;l->shaped_loaded++;
    }
    double center=u-4.0,i=0.0,q=0.0;
    long first=(long)ceil(center-4.0),last=(long)floor(center+4.0);
    if(first<0)first=0;
    for(long symbol=first;symbol<=last;symbol++){
        if((uint64_t)symbol>=l->shaped_loaded||l->shaped_loaded-(uint64_t)symbol>32)continue;
        double tap=rrc(center-(double)symbol);unsigned slot=(unsigned)((uint64_t)symbol&31u);
        i+=l->shaped_i[slot]*tap;q+=l->shaped_q[slot]*tap;
    }
    double p=2*M_PI*1800.0*(double)l->tx_samples/8000.0;
    double value=i*cos(p)-q*sin(p);if(value>32767.0)value=32767.0;if(value<-32768.0)value=-32768.0;
    l->tx_samples++;return(int16_t)lrint(value);
}
void v32_line_generate(struct v32_line*l,int16_t*out,size_t n){if(l->pulse_shaped){for(size_t k=0;k<n;k++)out[k]=shaped_sample(l);return;}for(size_t k=0;k<n;k++){if(l->tx_clock<=0){enum v32_carrier_state s=V32_STATE_A;if(l->th!=l->tt){s=(enum v32_carrier_state)l->tx[l->th];l->th=(l->th+1)&MASK;}point(s,&l->tx_i,&l->tx_q);l->tx_clock+=8000.0/2400.0;}double p=2*M_PI*1800.0*(double)l->tx_samples/8000.0;out[k]=(int16_t)(l->tx_i*cos(p)-l->tx_q*sin(p));l->tx_samples++;l->tx_clock-=1;}}
static enum v32_carrier_state decide(double i,double q){if(q>=0)return i>=0?V32_STATE_A:V32_STATE_B;return i<0?V32_STATE_C:V32_STATE_D;}
void v32_line_receive(struct v32_line*l,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double p=2*M_PI*1800.0*(double)l->rx_samples/8000.0,c=cos(p),s=sin(p),x=in[k];l->rx_i+=x*c;l->rx_q-=x*s;l->rx_cc+=c*c;l->rx_ss+=s*s;l->rx_cs+=c*s;l->rx_samples++;l->rx_clock++;if(l->rx_clock>=8000.0/2400.0){double det=l->rx_cc*l->rx_ss-l->rx_cs*l->rx_cs,i=l->rx_i,q=l->rx_q;if(fabs(det)>1e-9){i=(l->rx_ss*l->rx_i+l->rx_cs*l->rx_q)/det;q=(l->rx_cs*l->rx_i+l->rx_cc*l->rx_q)/det;}size_t next=(l->rt+1)&MASK;if(next!=l->rh){l->rx[l->rt]=(uint8_t)decide(i,q);l->rt=next;}l->rx_i=l->rx_q=l->rx_cc=l->rx_ss=l->rx_cs=0;l->rx_clock-=8000.0/2400.0;}}}
