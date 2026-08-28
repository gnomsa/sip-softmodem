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

unsigned v34_descramble_bit(v34_scrambler *s, unsigned input)
{
    unsigned output;
    if (s == NULL || (s->tap != 5u && s->tap != 18u))
        return 0;
    output = (input & 1u) ^ ((s->history >> (s->tap - 1u)) & 1u) ^
             ((s->history >> 22u) & 1u);
    s->history = ((s->history << 1u) | (input & 1u)) & 0x7fffffu;
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

static bool coded_bits4_phase(v34_scrambler *scrambler, const uint8_t *pattern,
                              unsigned pattern_bits, bool repeat,
                              unsigned *bit_index, unsigned *previous_rotation,
                              uint8_t *phase_pi_6)
{
    unsigned i1, i2, input, rotation;
    if (scrambler == NULL || bit_index == NULL || previous_rotation == NULL ||
        phase_pi_6 == NULL || pattern == NULL || pattern_bits < 2u ||
        (!repeat && *bit_index + 2u > pattern_bits))
        return false;
    i1 = v34_scramble_bit(scrambler, pattern[*bit_index % pattern_bits]);
    (*bit_index)++;
    i2 = v34_scramble_bit(scrambler, pattern[*bit_index % pattern_bits]);
    (*bit_index)++;
    input = 2u * i2 + i1;
    rotation = (input + *previous_rotation) & 3u;
    *previous_rotation = rotation;
    *phase_pi_6 = (uint8_t)((12u - 3u * rotation) % 12u);
    return true;
}

bool v34_j4_phase(v34_scrambler *scrambler, unsigned *bit_index,
                  unsigned *previous_rotation, uint8_t *phase_pi_6)
{
    static const uint8_t pattern[16] = {
        0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1
    };
    return coded_bits4_phase(scrambler, pattern, 16, true, bit_index,
                             previous_rotation, phase_pi_6);
}

bool v34_j_prime4_phase(v34_scrambler *scrambler, unsigned *bit_index,
                        unsigned *previous_rotation, uint8_t *phase_pi_6)
{
    static const uint8_t pattern[16] = {
        1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1
    };
    return coded_bits4_phase(scrambler, pattern, 16, false, bit_index,
                             previous_rotation, phase_pi_6);
}

bool v34_e4_phase(v34_scrambler *scrambler, unsigned *bit_index,
                  unsigned *previous_rotation, uint8_t *phase_pi_6)
{
    static const uint8_t ones[20] = {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };
    return coded_bits4_phase(scrambler, ones, 20, false, bit_index,
                             previous_rotation, phase_pi_6);
}

bool v34_packed_bits4_phase(v34_scrambler *scrambler, const uint8_t *bits,
                            unsigned bit_count, unsigned *bit_index,
                            unsigned *previous_rotation,
                            uint8_t *phase_pi_6)
{
    unsigned i1, i2, input, rotation;
    if (scrambler == NULL || bits == NULL || bit_index == NULL ||
        previous_rotation == NULL || phase_pi_6 == NULL ||
        *bit_index + 2u > bit_count)
        return false;
    i1 = v34_scramble_bit(scrambler,
                          (bits[*bit_index / 8u] >> (*bit_index % 8u)) & 1u);
    (*bit_index)++;
    i2 = v34_scramble_bit(scrambler,
                          (bits[*bit_index / 8u] >> (*bit_index % 8u)) & 1u);
    (*bit_index)++;
    input = 2u * i2 + i1;
    rotation = (input + *previous_rotation) & 3u;
    *previous_rotation = rotation;
    *phase_pi_6 = (uint8_t)((12u - 3u * rotation) % 12u);
    return true;
}
