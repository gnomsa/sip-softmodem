#include "v34_timing.h"

#include <stddef.h>

bool v34_symbol_clock_init(v34_symbol_clock *clock, v34_symbol_rate rate,
                           unsigned sample_rate)
{
    const v34_symbol_info *info = v34_get_symbol_info(rate);
    if (clock == NULL || info == NULL || sample_rate == 0)
        return false;
    clock->symbol_numerator = info->rate_num * 2400u;
    clock->sample_denominator = sample_rate * info->rate_den;
    clock->phase = 0;
    clock->samples = 0;
    clock->symbols = 0;
    return true;
}

bool v34_symbol_clock_tick(v34_symbol_clock *clock)
{
    if (clock == NULL || clock->sample_denominator == 0)
        return false;
    clock->samples++;
    clock->phase += clock->symbol_numerator;
    if (clock->phase >= clock->sample_denominator) {
        clock->phase -= clock->sample_denominator;
        clock->symbols++;
        return true;
    }
    return false;
}

uint64_t v34_samples_for_symbols(v34_symbol_rate rate, unsigned sample_rate,
                                 uint64_t symbols)
{
    const v34_symbol_info *info = v34_get_symbol_info(rate);
    uint64_t numerator;
    uint64_t denominator;
    if (info == NULL || sample_rate == 0 || symbols == 0)
        return 0;
    numerator = symbols * sample_rate * info->rate_den;
    denominator = info->rate_num * 2400u;
    return (numerator + denominator - 1u) / denominator;
}
