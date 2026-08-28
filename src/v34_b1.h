#ifndef SOFTMODEM_V34_B1_H
#define SOFTMODEM_V34_B1_H

#include "v34_framing.h"
#include "v34_training_symbols.h"

#include <stddef.h>

typedef struct {
    v34_frame_geometry geometry;
    v34_scrambler scrambler_after;
    uint16_t mapping_offset[V34_MAX_MAPPING_FRAMES + 1u];
    uint8_t bits[V34_MAX_DATA_FRAME_BITS];
} v34_b1_frame;

bool v34_b1_frame_init(v34_b1_frame *frame,
                       v34_symbol_rate symbol_rate,
                       unsigned data_rate,
                       bool call_modem);
size_t v34_b1_mapping_bits(const v34_b1_frame *frame,
                           unsigned mapping_frame);
const uint8_t *v34_b1_mapping_data(const v34_b1_frame *frame,
                                   unsigned mapping_frame);
bool v34_b1_v0(const v34_b1_frame *frame, unsigned interval_4d);

#endif
