#ifndef SIP_SOFTMODEM_V21_H
#define SIP_SOFTMODEM_V21_H
#include <stddef.h>
#include <stdint.h>

#define V21_RATE 8000
#define V21_BAUD 300
struct v21 {
    double tx_phase, tx_clock;
    uint8_t tx_queue[4096]; size_t tx_head, tx_tail;
    unsigned tx_frame; int tx_bits, tx_bit;
    double rx_mark_i,rx_mark_q,rx_space_i,rx_space_q,rx_clock,rx_phase;
    unsigned rx_frame; int rx_bits, rx_receiving;
    uint8_t rx_queue[4096]; size_t rx_head, rx_tail;
    int answer_role;
};
void v21_init(struct v21 *v);
void v21_set_answer_role(struct v21 *v,int answer);
size_t v21_write(struct v21 *v,const uint8_t *data,size_t length);
size_t v21_read(struct v21 *v,uint8_t *data,size_t capacity);
void v21_generate(struct v21 *v,int16_t *samples,size_t count);
void v21_receive(struct v21 *v,const int16_t *samples,size_t count);
#endif
