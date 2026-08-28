#include "v34_training_symbols.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    unsigned i;
    uint8_t phase, repeated;
    for (i = 0; i < V34_PP_PERIOD; ++i) {
        assert(v34_pp_phase(i, &phase));
        assert(v34_pp_phase(i + V34_PP_PERIOD, &repeated));
        assert(phase == repeated && phase < 12);
    }
    assert(v34_pp_phase(0, &phase) && phase == 0);
    assert(v34_pp_phase(4, &phase) && phase == 4);
    assert(v34_pp_phase(5, &phase) && phase == 5);
    assert(!v34_pp_phase(V34_PP_SYMBOLS, &phase));
    for (i = 0; i < 128; ++i) {
        assert(v34_s_phase(i, false, &phase));
        assert(phase == ((i & 1u) ? 3u : 0u));
    }
    assert(v34_s_phase(127, false, &phase) && phase == 3);
    assert(v34_s_phase(0, true, &phase) && phase == 6);
    assert(v34_s_phase(15, true, &phase) && phase == 9);
    {
        v34_scrambler call, answer;
        unsigned bit;
        v34_scrambler_init(&call, true);
        for (i = 0; i < 18; ++i)
            assert(v34_scramble_bit(&call, 1) == 1);
        bit = v34_scramble_bit(&call, 1);
        assert(bit == 0);
        v34_scrambler_init(&answer, false);
        for (i = 0; i < 5; ++i)
            assert(v34_scramble_bit(&answer, 1) == 1);
        assert(v34_scramble_bit(&answer, 1) == 0);
        v34_scrambler_init(&call, true);
        for (i = 0; i < 9; ++i) {
            assert(v34_trn4_phase(&call, &phase));
            assert(phase == 3);
        }
        assert(v34_trn4_phase(&call, &phase));
        assert(phase == 0);
        {
            unsigned bit_index = 0;
            unsigned rotation = 0;
            for (i = 0; i < 8; ++i)
                assert(v34_j4_phase(&call, &bit_index, &rotation, &phase));
            assert(bit_index == 16);
            assert(phase < 12 && phase % 3 == 0);
        }
        {
            unsigned bit_index = 0;
            unsigned rotation = 0;
            for (i = 0; i < 8; ++i)
                assert(v34_j_prime4_phase(&call, &bit_index, &rotation, &phase));
            assert(bit_index == 16);
            assert(!v34_j_prime4_phase(&call, &bit_index, &rotation, &phase));
            bit_index = 0;
            for (i = 0; i < 10; ++i)
                assert(v34_e4_phase(&call, &bit_index, &rotation, &phase));
            assert(bit_index == 20);
            assert(!v34_e4_phase(&call, &bit_index, &rotation, &phase));
        }
    }
    puts("v34 S/Sbar and PP symbol tests: ok");
    return 0;
}
