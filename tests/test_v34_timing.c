#include "v34_timing.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate) {
        v34_symbol_clock clock;
        const v34_symbol_info *info = v34_get_symbol_info((v34_symbol_rate)rate);
        unsigned i, boundaries = 0;
        assert(v34_symbol_clock_init(&clock, (v34_symbol_rate)rate, 8000));
        for (i = 0; i < 8000; ++i)
            if (v34_symbol_clock_tick(&clock)) boundaries++;
        assert(boundaries == info->rate_num * 2400u / info->rate_den);
        assert(clock.samples == 8000 && clock.symbols == boundaries);
        assert(v34_samples_for_symbols((v34_symbol_rate)rate, 8000, 128) > 0);
        {
            uint64_t short_count =
                v34_samples_for_symbols((v34_symbol_rate)rate, 8000, 128);
            uint64_t long_count =
                v34_samples_for_symbols((v34_symbol_rate)rate, 8000, 512);
            assert(long_count <= 4u * short_count);
            assert(4u * short_count - long_count <= 3u);
        }
    }
    assert(v34_samples_for_symbols(V34_SYMBOL_2400, 8000, 128) == 427);
    assert(v34_samples_for_symbols(V34_SYMBOL_3200, 8000, 128) == 320);
    puts("v34 fractional symbol timing tests: ok");
    return 0;
}
