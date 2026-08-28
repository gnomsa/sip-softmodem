#ifndef SIP_SOFTMODEM_V22BIS_H
#define SIP_SOFTMODEM_V22BIS_H
#include <stddef.h>
#include <stdint.h>
struct v22bis{uint8_t tq[4096],rq[4096];size_t th,tt,rh,rt;unsigned tf,ts,rs;int tb,to,role,quadrant;double phase,clock,tx_x,tx_y;uint64_t tx_samples,rx_samples;double ri,rqv,rclock,rphase;int previous_quadrant,have_quadrant,rbits,rreceiving;unsigned rframe;};
void v22bis_init(struct v22bis*v);void v22bis_set_answer_role(struct v22bis*v,int answer);
size_t v22bis_write(struct v22bis*v,const uint8_t*d,size_t n);size_t v22bis_read(struct v22bis*v,uint8_t*d,size_t n);
void v22bis_generate(struct v22bis*v,int16_t*out,size_t n);void v22bis_receive(struct v22bis*v,const int16_t*in,size_t n);
#endif
