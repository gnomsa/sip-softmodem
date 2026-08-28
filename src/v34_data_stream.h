#ifndef SOFTMODEM_V34_DATA_STREAM_H
#define SOFTMODEM_V34_DATA_STREAM_H

#include "v34_b1_stream.h"

typedef struct {
    v34_frame_geometry geometry;
    v34_mapping_parameters parameters;
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    v34_scrambler scrambler;
    v34_symbol_clock clock;
    v34_qam_tx tx;
    v34_mapped_frame mapped;
    v34_point interval[2];
    uint8_t input_bits[V34_MAX_SUPERFRAME_BITS];
    uint8_t frame_bits[V34_MAX_DATA_FRAME_BITS];
    uint16_t mapping_offset[V34_MAX_MAPPING_FRAMES + 1u];
    size_t input_count;
    size_t input_offset;
    unsigned data_frame;
    unsigned mapping_frame;
    unsigned interval_4d;
    unsigned symbol_in_4d;
    uint64_t symbols;
    uint64_t active_samples;
    bool expanded_shaping;
    bool complete;
} v34_data_stream;

bool v34_data_stream_init_after_b1(v34_data_stream *stream,
                                   const v34_b1_stream *b1,
                                   const uint8_t *input_bits,
                                   size_t input_count);
bool v34_data_stream_next_superframe(v34_data_stream *stream,
                                     const uint8_t *input_bits,
                                     size_t input_count);
size_t v34_data_stream_generate(v34_data_stream *stream,
                                uint8_t *pcma,
                                size_t count);
bool v34_data_stream_complete(const v34_data_stream *stream);

#endif
