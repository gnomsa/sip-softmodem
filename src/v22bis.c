#include "v22bis.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define QM 4095u
void v22bis_init(struct v22bis*v){memset(v,0,sizeof*v);v->role=1;v->quadrant=1;v->tx_x=v->tx_y=1;}
void v22bis_set_answer_role(struct v22bis*v,int a){v->role=a!=0;}
void v22bis_start_handshake(struct v22bis*v,int answer){v->handshake_enabled=1;v->role=answer!=0;v->probe_last=-1;v22_handshake_init(&v->hs,answer?V22_HS_ANSWER:V22_HS_CALLER);}
void v22bis_answer_sequence_complete(struct v22bis*v){v22_handshake_answer_sequence_complete(&v->hs);}
void v22bis_advance(struct v22bis*v,unsigned ms){if(v->handshake_enabled)v22_handshake_advance(&v->hs,ms);}
int v22bis_connected(const struct v22bis*v){return v->handshake_enabled?v22_handshake_connected(&v->hs):1;}
int v22bis_selected_rate(const struct v22bis*v){return v->handshake_enabled?v->hs.selected_rate:2400;}
size_t v22bis_write(struct v22bis*v,const uint8_t*d,size_t n){size_t z=0;while(z<n&&((v->tt+1)&QM)!=v->th){v->tq[v->tt]=d[z++];v->tt=(v->tt+1)&QM;}return z;}
size_t v22bis_read(struct v22bis*v,uint8_t*d,size_t n){size_t z=0;while(z<n&&v->rh!=v->rt){d[z++]=v->rq[v->rh];v->rh=(v->rh+1)&QM;}return z;}
static int raw(struct v22bis*v){if(!v->tb){if(v->th==v->tt)return 1;unsigned b=v->tq[v->th];v->th=(v->th+1)&QM;v->tf=(1u<<9)|(b<<1);v->tb=10;}return(v->tf>>(10-v->tb--))&1u;}
static int scr(struct v22bis*v,int b){int o=b^((v->ts>>13)&1u)^((v->ts>>16)&1u);v->ts=((v->ts<<1)|(unsigned)o)&0x1ffffu;return o;}
static void point(int q,int bits,double*x,double*y){int b2=(bits>>1)&1,b3=bits&1;double ax=1,ay=1;if(q==1){ax=b3?3:1;ay=b2?3:1;}if(q==2){ax=b2?3:1;ay=b3?3:1;}if(q==3){ax=b3?3:1;ay=b2?3:1;}if(q==4){ax=b2?3:1;ay=b3?3:1;}*x=(q==2||q==3)?-ax:ax;*y=(q==3||q==4)?-ay:ay;}
static int next_quadrant(int q,int a,int b){int delta=(a==0&&b==1)?0:(a==0&&b==0)?1:(a==1&&b==0)?2:3;return((q-1+delta)&3)+1;}
static void hs_symbol(struct v22bis*v,int*a,int*b,int*c,int*d){enum v22_hs_tx t=v->hs.tx;*c=*d=0;if(t==V22_TX_UNSCRAMBLED_ONES_1200){*a=*b=1;}else if(t==V22_TX_RATE_PROBE_0011){int x=(v->tx_symbols&1)?3:0;*a=(x>>1)&1;*b=x&1;}else if(t==V22_TX_SCRAMBLED_ONES_1200||t==V22_TX_SCRAMBLED_ONES_2400){*a=scr(v,1);*b=scr(v,1);*c=scr(v,1);*d=scr(v,1);}else{*a=scr(v,raw(v));*b=scr(v,raw(v));*c=scr(v,raw(v));*d=scr(v,raw(v));}}
void v22bis_generate(struct v22bis*v,int16_t*out,size_t n){for(size_t k=0;k<n;k++){if(v->handshake_enabled&&v->hs.tx==V22_TX_SILENCE){out[k]=0;v->tx_samples++;continue;}if(v->clock<=0){int a,b,c,d;if(v->handshake_enabled)hs_symbol(v,&a,&b,&c,&d);else{int training=v->tx_samples<16000;a=scr(v,training?1:raw(v));b=scr(v,training?1:raw(v));c=scr(v,training?1:raw(v));d=scr(v,training?1:raw(v));}v->quadrant=next_quadrant(v->quadrant,a,b);int use_2400=!v->handshake_enabled||v->hs.tx==V22_TX_SCRAMBLED_ONES_2400||v->hs.tx==V22_TX_DATA_2400;point(v->quadrant,use_2400?(c<<1)|d:0,&v->tx_x,&v->tx_y);v->tx_symbols++;v->clock+=8000.0/600.0;}double hz=v->role?2400.0:1200.0;double p=2*M_PI*hz*(double)v->tx_samples/8000.0;out[k]=(int16_t)((v->tx_x*cos(p)-v->tx_y*sin(p))*3300.0);v->tx_samples++;v->clock-=1;}}
static int descr(struct v22bis*v,int b){int o=b^((v->rs>>13)&1u)^((v->rs>>16)&1u);v->rs=((v->rs<<1)|(unsigned)b)&0x1ffffu;return o;}
static void uart(struct v22bis*v,int b){if(!v->rreceiving){if(!b){v->rreceiving=1;v->rbits=0;v->rframe=0;}return;}if(v->rbits<8)v->rframe|=(unsigned)b<<v->rbits++;else{if(b){size_t z=(v->rt+1)&QM;if(z!=v->rh){v->rq[v->rt]=(uint8_t)v->rframe;v->rt=z;}}v->rreceiving=0;}}
static int quadrant(double x,double y){return y>=0?(x>=0?1:2):(x<0?3:4);}
static void observe_handshake(struct v22bis*v,int a,int b,int c,int d){
    int pair=(a<<1)|b;
    if(v->hs.role==V22_HS_CALLER&&v->hs.tx==V22_TX_SILENCE){
        v->uns_symbols=pair==3?v->uns_symbols+1:0;
        if(v->uns_symbols==93)v22_handshake_event(&v->hs,V22_RX_UNSCRAMBLED_ONES,155);
        return;
    }
    int seek_probe=(v->hs.role==V22_HS_ANSWER&&v->hs.tx==V22_TX_UNSCRAMBLED_ONES_1200)||
        v->hs.tx==V22_TX_RATE_PROBE_0011||v->hs.tx==V22_TX_SCRAMBLED_ONES_1200;
    if(seek_probe&&!v->hs.selected_rate){
        if((pair==0||pair==3)&&pair!=v->probe_last)v->probe_symbols++;else v->probe_symbols=0;
        if(v->probe_symbols>v->probe_max)v->probe_max=v->probe_symbols;
        v->probe_last=pair;
        if(v->probe_symbols==30)v22_handshake_event(&v->hs,V22_RX_RATE_PROBE_0011,100);
    }
    /* Do not decide fallback before a delayed 00/11 reply has had time to
       traverse the jitter buffers in both directions. */
    if(v->hs.tx==V22_TX_SCRAMBLED_ONES_1200&&!v->hs.selected_rate&&
       v->hs.now_ms+300>=v->hs.deadline_ms){
        if(pair!=3)v->saw_scrambled_variation=1;
        int da=descr(v,a),db=descr(v,b);
        v->scrambled_ones=(v->saw_scrambled_variation&&da&&db)?v->scrambled_ones+2:0;
        if(v->scrambled_ones>=324)v22_handshake_event(&v->hs,V22_RX_SCRAMBLED_ONES_1200,270);
    }else if(v->hs.selected_rate==2400&&v->hs.tx>=V22_TX_SCRAMBLED_ONES_2400){
        int da=descr(v,a),db=descr(v,b),dc=descr(v,c),dd=descr(v,d);
        v->scrambled_ones=(da&&db&&dc&&dd)?v->scrambled_ones+4:0;
        if(v->scrambled_ones>=32)v22_handshake_event(&v->hs,V22_RX_SCRAMBLED_ONES_2400,14);
    }
}
static void decide(struct v22bis*v){int q=quadrant(v->ri,v->rqv),delta=(q-v->previous_quadrant+4)&3,a,b;if(delta==0){a=0;b=1;}else if(delta==1){a=0;b=0;}else if(delta==2){a=1;b=0;}else{a=1;b=1;}double ax=fabs(v->ri),ay=fabs(v->rqv);int c,d;if(q==1||q==3){d=ax>1.3;c=ay>1.3;}else{c=ax>1.3;d=ay>1.3;}if(v->handshake_enabled&&!v22_handshake_connected(&v->hs))observe_handshake(v,a,b,c,d);else{int da=descr(v,a),db=descr(v,b),dc=descr(v,c),dd=descr(v,d);int data_ready=v->handshake_enabled?v22_handshake_connected(&v->hs):v->rx_samples>16000;if(v->have_quadrant&&data_ready){uart(v,da);uart(v,db);if(!v->handshake_enabled||v->hs.selected_rate==2400){uart(v,dc);uart(v,dd);}}}v->previous_quadrant=q;v->have_quadrant=1;v->ri=v->rqv=0;}
void v22bis_receive(struct v22bis*v,const int16_t*in,size_t n){double hz=v->role?1200.0:2400.0;for(size_t k=0;k<n;k++){double s=in[k]/32768.0;v->ri+=s*cos(v->rphase);v->rqv-=s*sin(v->rphase);v->rphase+=2*M_PI*hz/8000.0;if(v->rphase>=2*M_PI)v->rphase-=2*M_PI;v->rclock++;v->rx_samples++;if(v->rclock>=8000.0/600.0){decide(v);v->rclock-=8000.0/600.0;}}}
