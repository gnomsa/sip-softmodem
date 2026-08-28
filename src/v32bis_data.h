#ifndef SIP_SOFTMODEM_V32BIS_DATA_H
#define SIP_SOFTMODEM_V32BIS_DATA_H
#include "v32_std.h"
#include "v32bis_trellis.h"
#include "v32bis_viterbi.h"
#include <stddef.h>
#include <stdint.h>
struct v32bis_data {
    int rate,info_bits;uint8_t tq[4096],rq[4096];size_t th,tt,rh,rt;
    unsigned tx_frame,rx_frame;int tx_frame_bits,rx_bits,rx_receiving;
    unsigned tx_previous,rx_previous;
    struct v32_std_scrambler tx_scr,rx_descr;
    struct v32bis_trellis trellis;struct v32bis_viterbi viterbi;
};
int v32bis_data_init(struct v32bis_data*d,enum v32_std_role role,int rate);
size_t v32bis_data_write(struct v32bis_data*d,const uint8_t*bytes,size_t count);
size_t v32bis_data_read(struct v32bis_data*d,uint8_t*bytes,size_t capacity);
unsigned v32bis_data_next(struct v32bis_data*d);
int v32bis_data_put(struct v32bis_data*d,double i,double q);
#endif
