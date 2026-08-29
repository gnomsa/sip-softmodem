#include "v32bis_qam.h"
#include "v32bis_map.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MASK 1023u
static unsigned labels_for_rate(int rate){return rate==7200?16:rate==9600?32:rate==12000?64:rate==14400?128:0;}
int v32bis_qam_init(struct v32bis_qam*q,int rate){unsigned labels=labels_for_rate(rate);if(!q||!labels)return-1;memset(q,0,sizeof *q);q->rate=rate;double energy=0;for(unsigned n=0;n<labels;n++){struct v32bis_point p;if(v32bis_map_point(rate,n,&p)<0)return-1;energy+=p.i*p.i+p.q*p.q;}q->gain=9000.0/sqrt(energy/labels);return 0;}
void v32bis_qam_set_pulse_shaped(struct v32bis_qam*q,int enabled)
{
    q->pulse_shaped=enabled!=0;
    if(q->pulse_shaped){
        q->rx_clock=0.6;
        q->rx_last_symbol=INT64_MIN;
    }
}
size_t v32bis_qam_write(struct v32bis_qam*q,const uint8_t*s,size_t n){size_t z=0;unsigned labels=labels_for_rate(q->rate);while(z<n&&((q->tt+1)&MASK)!=q->th){if(s[z]>=labels)break;struct v32bis_point p;if(v32bis_map_point(q->rate,s[z],&p)<0)break;q->tx[q->tt]=(struct v32bis_sample){p.i,p.q};z++;q->tt=(q->tt+1)&MASK;}return z;}
size_t v32bis_qam_write_carriers(struct v32bis_qam*q,const uint8_t*s,size_t n){static const double xy[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};size_t z=0;while(z<n&&((q->tt+1)&MASK)!=q->th){unsigned state=s[z++]&3u;q->tx[q->tt]=(struct v32bis_sample){xy[state][0]*8500.0/q->gain,xy[state][1]*8500.0/q->gain};q->tt=(q->tt+1)&MASK;}return z;}
size_t v32bis_qam_read(struct v32bis_qam*q,struct v32bis_sample*s,size_t n){size_t z=0;while(z<n&&q->rh!=q->rt){s[z++]=q->rx[q->rh];q->rh=(q->rh+1)&MASK;}return z;}
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
static int16_t shaped_sample(struct v32bis_qam*q)
{
    double u=(double)q->tx_samples*2400.0/8000.0;
    uint64_t required=(uint64_t)floor(u)+1;
    while(q->shaped_loaded<required){
        struct v32bis_sample p={0,0};
        if(q->th!=q->tt){p=q->tx[q->th];q->th=(q->th+1)&MASK;}
        unsigned slot=(unsigned)(q->shaped_loaded&31u);
        q->shaped_i[slot]=p.i*q->gain;q->shaped_q[slot]=p.q*q->gain;
        q->shaped_loaded++;
    }
    double center=u-4.0,i=0.0,v=0.0;
    long first=(long)ceil(center-4.0),last=(long)floor(center+4.0);
    if(first<0)first=0;
    for(long symbol=first;symbol<=last;symbol++){
        if((uint64_t)symbol>=q->shaped_loaded||q->shaped_loaded-(uint64_t)symbol>32)continue;
        double tap=rrc(center-(double)symbol);unsigned slot=(unsigned)((uint64_t)symbol&31u);
        i+=q->shaped_i[slot]*tap;v+=q->shaped_q[slot]*tap;
    }
    double phase=2*M_PI*1800.0*(double)q->tx_samples/8000.0;
    double sample=i*cos(phase)-v*sin(phase);
    if(sample>32767.0)sample=32767.0;
    if(sample<-32768.0)sample=-32768.0;
    q->tx_samples++;return(int16_t)lrint(sample);
}
void v32bis_qam_generate(struct v32bis_qam*q,int16_t*out,size_t n){if(q->pulse_shaped){for(size_t k=0;k<n;k++)out[k]=shaped_sample(q);return;}for(size_t k=0;k<n;k++){if(q->tx_clock<=0){struct v32bis_sample p={0,0};if(q->th!=q->tt){p=q->tx[q->th];q->th=(q->th+1)&MASK;}q->tx_i=p.i*q->gain;q->tx_q=p.q*q->gain;q->tx_clock+=8000.0/2400.0;}double phase=2*M_PI*1800.0*q->tx_samples/8000.0;double v=q->tx_i*cos(phase)-q->tx_q*sin(phase);if(v>32767)v=32767;if(v<-32768)v=-32768;out[k]=(int16_t)v;q->tx_samples++;q->tx_clock-=1;}}
static int matched_sample(struct v32bis_qam*q,int16_t sample,
                          double *out_i,double *out_q)
{
    enum{TAPS=28};
    double phase=2*M_PI*1800.0*(double)q->rx_samples/8000.0;
    double x=sample,bi=2.0*x*cos(phase),bq=-2.0*x*sin(phase);
    q->rx_fir_at=(q->rx_fir_at+1u)&63u;
    q->rx_fir_i[q->rx_fir_at]=bi;q->rx_fir_q[q->rx_fir_at]=bq;
    if(q->rx_fir_count<TAPS)q->rx_fir_count++;
    q->rx_samples++;
    if(q->rx_fir_count<TAPS)return 0;
    double i=0.0,v=0.0;
    for(unsigned tap=0;tap<TAPS;tap++){
        unsigned at=(q->rx_fir_at-tap)&63u;
        double h=rrc((double)tap*2400.0/8000.0-4.0);
        i+=q->rx_fir_i[at]*h;v+=q->rx_fir_q[at]*h;
    }
    *out_i=i;*out_q=v;return 1;
}
static void receive_shaped_selected(struct v32bis_qam*q,const int16_t*in,size_t n)
{
    for(size_t k=0;k<n;k++){
        double i,v;if(!matched_sample(q,in[k],&i,&v))continue;
        double symbol_position=((double)(q->rx_samples-1)*2400.0/8000.0)-8.0+
                               q->rx_clock;
        int64_t symbol=(int64_t)floor(symbol_position+0.5);
        if(symbol<=q->rx_last_symbol)continue;
        q->rx_last_symbol=symbol;
        size_t next=(q->rt+1)&MASK;
        if(next!=q->rh){q->rx[q->rt]=(struct v32bis_sample){i/q->gain,v/q->gain};q->rt=next;}
    }
}
static void receive_shaped(struct v32bis_qam*q,const int16_t*in,size_t n)
{
    receive_shaped_selected(q,in,n);
}
void v32bis_qam_receive(struct v32bis_qam*q,const int16_t*in,size_t n){if(q->pulse_shaped){receive_shaped(q,in,n);return;}for(size_t k=0;k<n;k++){double phase=2*M_PI*1800.0*q->rx_samples/8000.0,c=cos(phase),s=sin(phase),x=in[k];q->rx_i+=x*c;q->rx_q-=x*s;q->rx_cc+=c*c;q->rx_ss+=s*s;q->rx_cs+=c*s;q->rx_samples++;q->rx_clock++;if(q->rx_clock>=8000.0/2400.0){double det=q->rx_cc*q->rx_ss-q->rx_cs*q->rx_cs,i=q->rx_i,v=q->rx_q;if(fabs(det)>1e-9){i=(q->rx_ss*q->rx_i+q->rx_cs*q->rx_q)/det;v=(q->rx_cs*q->rx_i+q->rx_cc*q->rx_q)/det;}size_t next=(q->rt+1)&MASK;if(next!=q->rh){q->rx[q->rt]=(struct v32bis_sample){i/q->gain,v/q->gain};q->rt=next;}q->rx_i=q->rx_q=q->rx_cc=q->rx_ss=q->rx_cs=0;q->rx_clock-=8000.0/2400.0;}}}
void v32bis_qam_copy_receiver(struct v32bis_qam*dst,
                              const struct v32bis_qam*src)
{
    dst->rx_clock=src->rx_clock;dst->rx_i=src->rx_i;dst->rx_q=src->rx_q;
    dst->rx_cc=src->rx_cc;dst->rx_ss=src->rx_ss;dst->rx_cs=src->rx_cs;
    memcpy(dst->rx,src->rx,sizeof dst->rx);dst->rh=src->rh;dst->rt=src->rt;
    dst->rx_samples=src->rx_samples;
    memcpy(dst->rx_fir_i,src->rx_fir_i,sizeof dst->rx_fir_i);
    memcpy(dst->rx_fir_q,src->rx_fir_q,sizeof dst->rx_fir_q);
    dst->rx_fir_at=src->rx_fir_at;dst->rx_fir_count=src->rx_fir_count;
    dst->rx_last_symbol=src->rx_last_symbol;
}
