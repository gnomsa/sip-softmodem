#ifndef SIP_SOFTMODEM_V32_DATA_H
#define SIP_SOFTMODEM_V32_DATA_H
#include "v32_training.h"
#include <stddef.h>
#include <stdint.h>
struct v32_data {
    uint8_t tq[4096],rq[4096];size_t th,tt,rh,rt;unsigned frame;int frame_bits;
    unsigned rx_frame;int rx_bits,rx_receiving,tx_previous,rx_previous;
    struct v32_std_scrambler tx_scr,rx_descr;
};
void v32_data_init(struct v32_data*d,enum v32_std_role local_role,enum v32_carrier_state previous_tx,enum v32_carrier_state previous_rx);
size_t v32_data_write(struct v32_data*d,const uint8_t*bytes,size_t count);
size_t v32_data_read(struct v32_data*d,uint8_t*bytes,size_t capacity);
enum v32_carrier_state v32_data_next_4800(struct v32_data*d);
void v32_data_put_4800(struct v32_data*d,enum v32_carrier_state state);
#endif
