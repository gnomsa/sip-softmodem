#ifndef SOFTMODEM_V34_TRELLIS_H
#define SOFTMODEM_V34_TRELLIS_H

#include "v34_data_mapper.h"

typedef enum {
    V34_TRELLIS_16 = 0,
    V34_TRELLIS_32 = 1,
    V34_TRELLIS_64 = 2
} v34_trellis_kind;

typedef struct {
    v34_trellis_kind kind;
    uint8_t state;
} v34_trellis_encoder;

bool v34_trellis_init(v34_trellis_encoder *encoder, v34_trellis_kind kind);
uint8_t v34_trellis_output(const v34_trellis_encoder *encoder);
uint8_t v34_trellis_put(v34_trellis_encoder *encoder,
                        uint8_t y4, uint8_t y3, uint8_t y2, uint8_t y1);
bool v34_subset_label(v34_point point, uint8_t *label);
bool v34_subset_pair_bits(uint8_t first_label, uint8_t second_label,
                          uint8_t *y4, uint8_t *y3,
                          uint8_t *y2, uint8_t *y1);
uint8_t v34_modulo_bit(v34_point first_c, v34_point second_c);
bool v34_encode_4d_zero_precoder(v34_trellis_encoder *encoder,
                                 v34_point first_quarter,
                                 v34_point second_quarter,
                                 uint8_t z,
                                 uint8_t i1,
                                 uint8_t v0,
                                 v34_point output[2]);

#endif
