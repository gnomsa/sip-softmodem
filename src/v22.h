#ifndef SIP_SOFTMODEM_V22_H
#define SIP_SOFTMODEM_V22_H
#include <stddef.h>
#include <stdint.h>
#define V22_RATE 8000
struct v22 {
    uint8_t tx_queue[4096],rx_queue[4096];size_t tx_head,tx_tail,rx_head,rx_tail;
    unsigned tx_frame,tx_scrambler,rx_descrambler;int tx_bits,tx_ones;
    double tx_phase,tx_clock;uint64_t tx_samples;
    double rx_i,rx_q,rx_clock,rx_phase,rx_previous_phase;uint64_t rx_samples;int rx_have_phase;
    unsigned rx_frame;int rx_bits,rx_receiving;
};
void v22_init(struct v22 *v);
size_t v22_write(struct v22 *v,const uint8_t *data,size_t length);
size_t v22_read(struct v22 *v,uint8_t *data,size_t capacity);
void v22_generate(struct v22 *v,int16_t *samples,size_t count);
void v22_receive(struct v22 *v,const int16_t *samples,size_t count);
#endif
