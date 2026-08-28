#ifndef SIP_SOFTMODEM_V42_STREAM_H
#define SIP_SOFTMODEM_V42_STREAM_H
#include "v42_hdlc.h"

#define V42_STREAM_BITS 16384
#define V42_STREAM_FRAMES 8
struct v42_stream_frame {uint8_t bits[V42_HDLC_MAX_BITS];size_t count;};
struct v42_stream {
    uint8_t tx[V42_STREAM_BITS];size_t tx_head,tx_tail;
    uint8_t assembling[V42_HDLC_MAX_BITS];size_t assembling_count;
    struct v42_stream_frame frames[V42_STREAM_FRAMES];size_t frame_head,frame_tail;
    int in_frame;
};
void v42_stream_init(struct v42_stream*s);
size_t v42_stream_send(struct v42_stream*s,const uint8_t*bits,size_t count);
size_t v42_stream_tx_bytes(struct v42_stream*s,uint8_t*out,size_t capacity);
size_t v42_stream_rx_bytes(struct v42_stream*s,const uint8_t*bytes,size_t count);
size_t v42_stream_receive(struct v42_stream*s,uint8_t*bits,size_t capacity);
#endif
