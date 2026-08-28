#include "v32_data.h"
#include <string.h>
#define MASK 4095u
static const enum v32_carrier_state pair_state[4]={V32_STATE_A,V32_STATE_B,V32_STATE_D,V32_STATE_C};
static unsigned state_pair(enum v32_carrier_state s){static const unsigned p[4]={0,1,3,2};return p[s&3];}
void v32_data_init(struct v32_data*d,enum v32_std_role role,enum v32_carrier_state ptx,enum v32_carrier_state prx){memset(d,0,sizeof *d);d->tx_previous=state_pair(ptx);d->rx_previous=state_pair(prx);v32_std_scrambler_init(&d->tx_scr,role);v32_std_scrambler_init(&d->rx_descr,role==V32_STD_CALL?V32_STD_ANSWER:V32_STD_CALL);}
size_t v32_data_write(struct v32_data*d,const uint8_t*b,size_t n){size_t z=0;while(z<n&&((d->tt+1)&MASK)!=d->th){d->tq[d->tt]=b[z++];d->tt=(d->tt+1)&MASK;}return z;}
size_t v32_data_read(struct v32_data*d,uint8_t*b,size_t n){size_t z=0;while(z<n&&d->rh!=d->rt){b[z++]=d->rq[d->rh];d->rh=(d->rh+1)&MASK;}return z;}
static int tx_bit(struct v32_data*d){if(!d->frame_bits){if(d->th==d->tt)return 1;unsigned b=d->tq[d->th];d->th=(d->th+1)&MASK;d->frame=(1u<<9)|(b<<1);d->frame_bits=10;}return(d->frame>>(10-d->frame_bits--))&1;}
enum v32_carrier_state v32_data_next_4800(struct v32_data*d){int a=v32_std_scramble(&d->tx_scr,tx_bit(d)),b=v32_std_scramble(&d->tx_scr,tx_bit(d));d->tx_previous=(int)v32_std_diff_encode((unsigned)((a<<1)|b),(unsigned)d->tx_previous);return pair_state[d->tx_previous];}
static void uart(struct v32_data*d,int bit){if(!d->rx_receiving){if(!bit){d->rx_receiving=1;d->rx_bits=0;d->rx_frame=0;}return;}if(d->rx_bits<8)d->rx_frame|=(unsigned)bit<<d->rx_bits++;else{if(bit){size_t next=(d->rt+1)&MASK;if(next!=d->rh){d->rq[d->rt]=(uint8_t)d->rx_frame;d->rt=next;}}d->rx_receiving=0;}}
void v32_data_put_4800(struct v32_data*d,enum v32_carrier_state state){unsigned current=state_pair(state),q=0;for(;q<4;q++)if(v32_std_diff_encode(q,(unsigned)d->rx_previous)==current)break;d->rx_previous=(int)current;if(q==4)return;uart(d,v32_std_descramble(&d->rx_descr,(q>>1)&1));uart(d,v32_std_descramble(&d->rx_descr,q&1));}
