#include "v22bis.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define QM 4095u
void v22bis_init(struct v22bis*v){memset(v,0,sizeof*v);v->role=1;v->quadrant=1;v->tx_x=v->tx_y=1;}
void v22bis_set_answer_role(struct v22bis*v,int a){v->role=a!=0;}
size_t v22bis_write(struct v22bis*v,const uint8_t*d,size_t n){size_t z=0;while(z<n&&((v->tt+1)&QM)!=v->th){v->tq[v->tt]=d[z++];v->tt=(v->tt+1)&QM;}return z;}
size_t v22bis_read(struct v22bis*v,uint8_t*d,size_t n){size_t z=0;while(z<n&&v->rh!=v->rt){d[z++]=v->rq[v->rh];v->rh=(v->rh+1)&QM;}return z;}
static int raw(struct v22bis*v){if(!v->tb){if(v->th==v->tt)return 1;unsigned b=v->tq[v->th];v->th=(v->th+1)&QM;v->tf=(1u<<9)|(b<<1);v->tb=10;}return(v->tf>>(10-v->tb--))&1u;}
static int scr(struct v22bis*v,int b){int o=b^((v->ts>>13)&1u)^((v->ts>>16)&1u);v->ts=((v->ts<<1)|(unsigned)o)&0x1ffffu;return o;}
static void point(int q,int bits,double*x,double*y){int b2=(bits>>1)&1,b3=bits&1;double ax=1,ay=1;if(q==1){ax=b3?3:1;ay=b2?3:1;}if(q==2){ax=b2?3:1;ay=b3?3:1;}if(q==3){ax=b3?3:1;ay=b2?3:1;}if(q==4){ax=b2?3:1;ay=b3?3:1;}*x=(q==2||q==3)?-ax:ax;*y=(q==3||q==4)?-ay:ay;}
static int next_quadrant(int q,int a,int b){int delta=(a==0&&b==1)?0:(a==0&&b==0)?1:(a==1&&b==0)?2:3;return((q-1+delta)&3)+1;}
void v22bis_generate(struct v22bis*v,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(v->clock<=0){int training=v->tx_samples<16000;int a=scr(v,training?1:raw(v)),b=scr(v,training?1:raw(v)),c=scr(v,training?1:raw(v)),d=scr(v,training?1:raw(v));v->quadrant=next_quadrant(v->quadrant,a,b);point(v->quadrant,(c<<1)|d,&v->tx_x,&v->tx_y);v->clock+=8000.0/600.0;}double hz=v->role?2400.0:1200.0;double p=2*M_PI*hz*(double)v->tx_samples/8000.0;out[k]=(int16_t)((v->tx_x*cos(p)-v->tx_y*sin(p))*3300.0);v->tx_samples++;v->clock-=1;}}
static int descr(struct v22bis*v,int b){int o=b^((v->rs>>13)&1u)^((v->rs>>16)&1u);v->rs=((v->rs<<1)|(unsigned)b)&0x1ffffu;return o;}
static void uart(struct v22bis*v,int b){if(!v->rreceiving){if(!b){v->rreceiving=1;v->rbits=0;v->rframe=0;}return;}if(v->rbits<8)v->rframe|=(unsigned)b<<v->rbits++;else{if(b){size_t z=(v->rt+1)&QM;if(z!=v->rh){v->rq[v->rt]=(uint8_t)v->rframe;v->rt=z;}}v->rreceiving=0;}}
static int quadrant(double x,double y){return y>=0?(x>=0?1:2):(x<0?3:4);}
static void decide(struct v22bis*v){int q=quadrant(v->ri,v->rqv),delta=(q-v->previous_quadrant+4)&3,a,b;if(delta==0){a=0;b=1;}else if(delta==1){a=0;b=0;}else if(delta==2){a=1;b=0;}else{a=1;b=1;}double ax=fabs(v->ri),ay=fabs(v->rqv);int c,d;if(q==1||q==3){d=ax>1.3;c=ay>1.3;}else{c=ax>1.3;d=ay>1.3;}int da=descr(v,a),db=descr(v,b),dc=descr(v,c),dd=descr(v,d);if(v->have_quadrant&&v->rx_samples>16000){uart(v,da);uart(v,db);uart(v,dc);uart(v,dd);}v->previous_quadrant=q;v->have_quadrant=1;v->ri=v->rqv=0;}
void v22bis_receive(struct v22bis*v,const int16_t*in,size_t n){double hz=v->role?1200.0:2400.0;for(size_t k=0;k<n;k++){double s=in[k]/32768.0;v->ri+=s*cos(v->rphase);v->rqv-=s*sin(v->rphase);v->rphase+=2*M_PI*hz/8000.0;if(v->rphase>=2*M_PI)v->rphase-=2*M_PI;v->rclock++;v->rx_samples++;if(v->rclock>=8000.0/600.0){decide(v);v->rclock-=8000.0/600.0;}}}
