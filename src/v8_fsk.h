#ifndef SIP_SOFTMODEM_V8_FSK_H
#define SIP_SOFTMODEM_V8_FSK_H
#include <stddef.h>
#include <stdint.h>
struct v8_fsk {
    uint8_t tx[2048],rx[4096];size_t tx_count,tx_at,rx_head,rx_tail;
    double tx_phase,tx_clock,rx_phase,rx_clock,mi,mq,si,sq;
    int high_channel;
};
void v8_fsk_init(struct v8_fsk *f,int high_channel);
size_t v8_fsk_set_sequence(struct v8_fsk *f,const uint8_t *bits,size_t count);
void v8_fsk_generate(struct v8_fsk *f,int16_t *samples,size_t count);
void v8_fsk_receive(struct v8_fsk *f,const int16_t *samples,size_t count);
size_t v8_fsk_read(struct v8_fsk *f,uint8_t *bits,size_t capacity);
#endif
