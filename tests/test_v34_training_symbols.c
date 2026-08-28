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
    puts("v34 S/Sbar and PP symbol tests: ok");
    return 0;
}
