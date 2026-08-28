#ifndef SOFTMODEM_V34_DATA_DECODER_H
#define SOFTMODEM_V34_DATA_DECODER_H

#include "v34_trellis.h"

typedef struct {
    v34_mapping_parameters parameters;
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    bool expanded_shaping;
} v34_data_decoder;

bool v34_data_decoder_init(v34_data_decoder *decoder,
                           const v34_mapping_parameters *parameters,
                           v34_trellis_kind trellis,
                           bool expanded_shaping);
bool v34_quarter_decode(const v34_constellation *constellation,
                        v34_point point,
                        uint8_t *clockwise_rotation,
                        uint16_t *index);
bool v34_slice_iq(double in_phase, double quadrature,
                  double coordinate_scale, v34_point *point);
bool v34_decode_mapping_frame(v34_data_decoder *decoder,
                              v34_point received[4][2],
                              const uint8_t v0[4],
                              bool high_frame,
                              uint8_t *scrambled_bits,
                              size_t *bit_count);

#endif
