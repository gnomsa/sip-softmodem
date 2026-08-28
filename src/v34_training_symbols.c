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

void v34_scrambler_init(v34_scrambler *scrambler, bool call_modem)
{
    if (scrambler == NULL)
        return;
    scrambler->history = 0;
    scrambler->tap = call_modem ? 18u : 5u;
}

unsigned v34_scramble_bit(v34_scrambler *scrambler, unsigned input)
{
    unsigned output;
    if (scrambler == NULL || (scrambler->tap != 5u && scrambler->tap != 18u))
        return 0;
    output = (input & 1u) ^
             ((scrambler->history >> (scrambler->tap - 1u)) & 1u) ^
             ((scrambler->history >> 22u) & 1u);
    scrambler->history = ((scrambler->history << 1u) | output) & 0x7fffffu;
    return output;
}

bool v34_trn4_phase(v34_scrambler *scrambler, uint8_t *phase_pi_6)
{
    unsigned i1, i2, index;
    if (scrambler == NULL || phase_pi_6 == NULL)
        return false;
    i1 = v34_scramble_bit(scrambler, 1u);
    i2 = v34_scramble_bit(scrambler, 1u);
    index = 2u * i2 + i1;
    *phase_pi_6 = (uint8_t)((12u - 3u * index) % 12u);
    return true;
}
