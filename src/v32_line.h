#ifndef SIP_SOFTMODEM_V32_LINE_H
#define SIP_SOFTMODEM_V32_LINE_H
#include "v32_training.h"
#include <stddef.h>
#include <stdint.h>
struct v32_line {
    uint8_t tx[4096],rx[4096];size_t th,tt,rh,rt;
    double tx_clock,tx_i,tx_q,rx_clock,rx_i,rx_q,rx_cc,rx_ss,rx_cs;
    uint64_t tx_samples,rx_samples;
};
void v32_line_init(struct v32_line*l);
size_t v32_line_write(struct v32_line*l,const enum v32_carrier_state*s,size_t count);
size_t v32_line_read(struct v32_line*l,enum v32_carrier_state*s,size_t capacity);
void v32_line_generate(struct v32_line*l,int16_t*out,size_t count);
void v32_line_receive(struct v32_line*l,const int16_t*in,size_t count);
#endif
