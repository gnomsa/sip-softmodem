#ifndef SOFTMODEM_V34_MP_H
#define SOFTMODEM_V34_MP_H
#include <stdbool.h>
#include <stdint.h>
#define V34_MP0_BITS 88u
#define V34_MP0_BYTES 11u
typedef struct {
    uint8_t call_to_answer_rate_2400;
    uint8_t answer_to_call_rate_2400;
    bool auxiliary_channel;
    uint8_t trellis_encoder;
    bool nonlinear_encoder;
    bool expanded_shaping;
    bool acknowledge;
    uint16_t rate_mask;
    bool asymmetric_rates;
} v34_mp0;
bool v34_mp0_encode(const v34_mp0 *mp, uint8_t frame[V34_MP0_BYTES]);
bool v34_mp0_decode(const uint8_t frame[V34_MP0_BYTES], v34_mp0 *mp);
typedef struct { unsigned call_to_answer; unsigned answer_to_call; bool asymmetric; } v34_final_rates;
bool v34_mp0_negotiate_rates(const v34_mp0 *local, const v34_mp0 *remote,
                             unsigned maximum_rate, v34_final_rates *rates);
#endif
