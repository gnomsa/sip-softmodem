#ifndef SOFTMODEM_V34_FRAMING_H
#define SOFTMODEM_V34_FRAMING_H
#include "v34_caps.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    unsigned data_frames_per_superframe;
    unsigned mapping_frames_per_data_frame;
    unsigned bits_per_data_frame;
    unsigned high_frame_bits;
    unsigned high_frame_count;
    uint16_t switching_pattern;
} v34_frame_geometry;
bool v34_frame_geometry_init(v34_frame_geometry *geometry,
                             v34_symbol_rate symbol_rate,
                             unsigned data_rate);
bool v34_mapping_frame_high(const v34_frame_geometry *geometry,
                            unsigned mapping_frame);
#endif
