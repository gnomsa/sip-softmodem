#ifndef SOFTMODEM_V34_B1_STREAM_H
#define SOFTMODEM_V34_B1_STREAM_H

#include "v34_b1.h"
#include "v34_qam_tx.h"
#include "v34_timing.h"
#include "v34_trellis.h"

typedef struct {
    v34_b1_frame b1;
    v34_mapping_parameters parameters;
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    v34_symbol_clock clock;
    v34_qam_tx tx;
    v34_mapped_frame mapped;
    v34_point interval[2];
    unsigned mapping_frame;
    unsigned interval_4d;
    unsigned symbol_in_4d;
    uint64_t active_samples;
    bool expanded_shaping;
    bool complete;
} v34_b1_stream;

bool v34_b1_stream_init(v34_b1_stream *stream,
                        v34_symbol_rate symbol_rate,
                        unsigned data_rate,
                        bool call_modem,
                        v34_trellis_kind trellis,
                        bool expanded_shaping,
                        bool high_carrier,
                        unsigned sample_rate,
                        double coordinate_scale);
size_t v34_b1_stream_generate(v34_b1_stream *stream,
                              uint8_t *pcma,
                              size_t count);
bool v34_b1_stream_complete(const v34_b1_stream *stream);
uint64_t v34_b1_stream_symbols(const v34_b1_stream *stream);

#endif
