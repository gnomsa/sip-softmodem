#ifndef SOFTMODEM_V34_PHASE3_STREAM_H
#define SOFTMODEM_V34_PHASE3_STREAM_H

#include "v34_phase3.h"
#include "v34_timing.h"
#include "v34_training_symbols.h"
#include "v34_training_tx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    v34_phase3_plan plan;
    v34_phase3_cursor cursor;
    v34_symbol_clock clock;
    v34_training_tx tx;
    v34_scrambler scrambler;
    v34_phase3_role role;
    unsigned sample_rate;
    uint32_t event_samples;
    uint32_t event_symbol;
    size_t active_samples;
} v34_phase3_stream;

bool v34_phase3_stream_init(v34_phase3_stream *stream, v34_phase3_role role,
                            uint8_t md_length_35ms, v34_symbol_rate rate,
                            bool high_carrier, unsigned sample_rate,
                            double amplitude);
size_t v34_phase3_stream_generate(v34_phase3_stream *stream, uint8_t *pcma,
                                  size_t count);
bool v34_phase3_stream_complete(const v34_phase3_stream *stream);

#endif
