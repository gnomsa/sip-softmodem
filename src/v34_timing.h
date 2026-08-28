#ifndef SOFTMODEM_V34_TIMING_H
#define SOFTMODEM_V34_TIMING_H

#include "v34_caps.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t symbol_numerator;
    uint32_t sample_denominator;
    uint32_t phase;
    uint64_t samples;
    uint64_t symbols;
} v34_symbol_clock;

bool v34_symbol_clock_init(v34_symbol_clock *clock, v34_symbol_rate rate,
                           unsigned sample_rate);
bool v34_symbol_clock_tick(v34_symbol_clock *clock);
uint64_t v34_samples_for_symbols(v34_symbol_rate rate, unsigned sample_rate,
                                 uint64_t symbols);

#endif
