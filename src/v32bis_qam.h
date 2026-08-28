#ifndef SIP_SOFTMODEM_V32BIS_QAM_H
#define SIP_SOFTMODEM_V32BIS_QAM_H
#include <stddef.h>
#include <stdint.h>
struct v32bis_sample {double i,q;};
struct v32bis_qam {int rate;double gain,tx_clock,tx_i,tx_q,rx_clock,rx_i,rx_q,rx_cc,rx_ss,rx_cs;uint8_t tx[4096];struct v32bis_sample rx[4096];size_t th,tt,rh,rt;uint64_t tx_samples,rx_samples;};
int v32bis_qam_init(struct v32bis_qam*q,int rate);
size_t v32bis_qam_write(struct v32bis_qam*q,const uint8_t*labels,size_t count);
size_t v32bis_qam_read(struct v32bis_qam*q,struct v32bis_sample*points,size_t capacity);
void v32bis_qam_generate(struct v32bis_qam*q,int16_t*out,size_t count);
void v32bis_qam_receive(struct v32bis_qam*q,const int16_t*in,size_t count);
#endif
