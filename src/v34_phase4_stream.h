#ifndef SOFTMODEM_V34_PHASE4_STREAM_H
#define SOFTMODEM_V34_PHASE4_STREAM_H

#include "v34_phase4.h"
#include "v34_timing.h"
#include "v34_training_tx.h"

#include <stddef.h>

typedef struct {
    v34_phase4 phase4;
    v34_symbol_clock clock;
    v34_training_tx tx;
    uint64_t symbols;
    uint64_t samples;
    bool complete;
} v34_phase4_stream;

bool v34_phase4_stream_init(v34_phase4_stream *stream,
                            bool call_modem,
                            const v34_scrambler *phase3_scrambler,
                            unsigned phase3_rotation,
                            const v34_mp0 *mp,
                            v34_symbol_rate symbol_rate,
                            bool high_carrier,
                            unsigned sample_rate,
                            double amplitude);
size_t v34_phase4_stream_generate(v34_phase4_stream *stream,
                                  uint8_t *pcma, size_t count);
bool v34_phase4_stream_complete(const v34_phase4_stream *stream);

#endif
