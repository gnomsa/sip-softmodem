#ifndef SIP_SOFTMODEM_V32_QAM_H
#define SIP_SOFTMODEM_V32_QAM_H
#include <stddef.h>
#include <stdint.h>
struct v32_qam {uint8_t tx[4096],rx[4096];size_t th,tt,rh,rt;double tx_clock,tx_i,tx_q,rx_clock,rx_i,rx_q,rx_cc,rx_ss,rx_cs;uint64_t tx_samples,rx_samples;};
void v32_qam_init(struct v32_qam*q);
size_t v32_qam_write(struct v32_qam*q,const uint8_t*symbols,size_t count);
size_t v32_qam_read(struct v32_qam*q,uint8_t*symbols,size_t capacity);
void v32_qam_generate(struct v32_qam*q,int16_t*out,size_t count);
void v32_qam_receive(struct v32_qam*q,const int16_t*in,size_t count);
#endif
