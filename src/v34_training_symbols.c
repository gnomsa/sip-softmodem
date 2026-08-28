#include "v34_training_symbols.h"

#include <stddef.h>

bool v34_pp_phase(unsigned symbol, uint8_t *phase_pi_6)
{
    unsigned i, k, l, phase;
    if (phase_pi_6 == NULL || symbol >= V34_PP_SYMBOLS)
        return false;
    i = symbol % V34_PP_PERIOD;
    k = i / 4u;
    l = i % 4u;
    phase = k * l;
    if (k % 3u == 1u)
        phase += 4u;
    *phase_pi_6 = (uint8_t)(phase % 12u);
    return true;
}

bool v34_s_phase(unsigned symbol, bool reversed, uint8_t *phase_pi_6)
{
    if (phase_pi_6 == NULL)
        return false;
    if (reversed)
        *phase_pi_6 = (uint8_t)((symbol & 1u) ? 9u : 6u);
    else
        *phase_pi_6 = (uint8_t)((symbol & 1u) ? 3u : 0u);
    return true;
}
