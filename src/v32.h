#ifndef SIP_SOFTMODEM_V32_H
#define SIP_SOFTMODEM_V32_H
#include <stddef.h>
#include <stdint.h>
struct v32 { uint8_t tq[4096],rq[4096];size_t th,tt,rh,rt;int rate,bits;unsigned tf;int tb,rx_bits,rx_receiving;unsigned rframe;double tx_clock,rx_clock,phase,carrier_i,carrier_q;uint64_t tx_samples,rx_samples; };
void v32_init(struct v32*v,int rate);size_t v32_write(struct v32*v,const uint8_t*d,size_t n);size_t v32_read(struct v32*v,uint8_t*d,size_t n);void v32_generate(struct v32*v,int16_t*out,size_t n);void v32_receive(struct v32*v,const int16_t*in,size_t n);
#endif
