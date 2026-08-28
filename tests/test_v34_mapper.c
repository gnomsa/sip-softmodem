#include "v34_b1.h"
#include "v34_mapper.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void check_parameters(void)
{
    v34_mapping_parameters p;

    assert(v34_mapping_parameters_init(&p, 8));
    assert(p.shell_bits == 0 && p.q_bits == 0);
    assert(p.minimum_rings == 1 && p.expanded_rings == 1);
    assert(p.minimum_points == 4 && p.expanded_points == 4);

    assert(v34_mapping_parameters_init(&p, 52));
    assert(p.shell_bits == 24 && p.q_bits == 2);
    assert(p.minimum_rings == 8 && p.expanded_rings == 10);
    assert(p.minimum_points == 128 && p.expanded_points == 160);

    assert(v34_mapping_parameters_init(&p, 79));
    assert(p.shell_bits == 27 && p.q_bits == 5);
    assert(p.minimum_rings == 11 && p.expanded_rings == 13);
    assert(p.minimum_points == 1408 && p.expanded_points == 1664);
    assert(!v34_mapping_parameters_init(&p, 7));
    assert(!v34_mapping_parameters_init(&p, 80));
}

static void check_large_parser(void)
{
    v34_b1_frame b1;
    v34_mapping_parameters p;
    v34_parsed_mapping_frame parsed;
    const uint8_t *input;
    unsigned j;
    unsigned symbol;
    unsigned bit;
    size_t offset;

    assert(v34_b1_frame_init(&b1, V34_SYMBOL_3000, 19200, true));
    assert(v34_mapping_parameters_init(&p, b1.geometry.high_frame_bits));

    input = v34_b1_mapping_data(&b1, 0);
    assert(v34_b1_mapping_bits(&b1, 0) == 51);
    assert(v34_parse_mapping_frame(&p, input, 51, &parsed));
    assert(parsed.low_frame && parsed.shell_bit_count == 24);
    for (bit = 0; bit < 23; ++bit)
        assert(parsed.shell[bit] == input[bit]);
    assert(parsed.shell[23] == 0);
    offset = 23;
    for (j = 0; j < 4; ++j) {
        for (bit = 0; bit < 3; ++bit)
            assert(parsed.i[j][bit] == input[offset++]);
        for (symbol = 0; symbol < 2; ++symbol)
            for (bit = 0; bit < 2; ++bit)
                assert(parsed.q[j][symbol][bit] == input[offset++]);
    }
    assert(offset == 51);

    input = v34_b1_mapping_data(&b1, 4);
    assert(v34_b1_mapping_bits(&b1, 4) == 52);
    assert(v34_parse_mapping_frame(&p, input, 52, &parsed));
    assert(!parsed.low_frame);
    for (bit = 0; bit < 24; ++bit)
        assert(parsed.shell[bit] == input[bit]);
}

static void check_small_parser(void)
{
    v34_b1_frame b1;
    v34_mapping_parameters p;
    v34_parsed_mapping_frame parsed;
    const uint8_t *input;
    unsigned j;
    size_t offset = 0;

    assert(v34_b1_frame_init(&b1, V34_SYMBOL_3429, 4800, false));
    assert(b1.geometry.high_frame_bits == 12);
    assert(v34_mapping_parameters_init(&p, 12));
    input = v34_b1_mapping_data(&b1, 0);
    assert(v34_b1_mapping_bits(&b1, 0) == 11);
    assert(v34_parse_mapping_frame(&p, input, 11, &parsed));
    assert(parsed.low_frame && parsed.shell_bit_count == 0);
    for (j = 0; j < 4; ++j) {
        assert(parsed.i[j][0] == input[offset++]);
        assert(parsed.i[j][1] == input[offset++]);
        if (j < 3)
            assert(parsed.i[j][2] == input[offset++]);
        else
            assert(parsed.i[j][2] == 0);
    }
    assert(offset == 11);

    assert(v34_b1_frame_init(&b1, V34_SYMBOL_2400, 2400, true));
    assert(v34_mapping_parameters_init(&p, 8));
    assert(v34_parse_mapping_frame(&p, v34_b1_mapping_data(&b1, 0), 8,
                                   &parsed));
    for (j = 0; j < 4; ++j)
        assert(parsed.i[j][2] == 0);
}

static uint64_t test_g2(unsigned rings, unsigned p)
{
    unsigned maximum = 2u * (rings - 1u);
    unsigned distance;

    if (p > maximum)
        return 0;
    distance = p > rings - 1u ? p - (rings - 1u) : rings - 1u - p;
    return rings - distance;
}

static uint64_t test_g4(unsigned rings, unsigned p)
{
    uint64_t count = 0;
    unsigned i;

    for (i = 0; i <= p; ++i)
        count += test_g2(rings, i) * test_g2(rings, p - i);
    return count;
}

static uint64_t test_g8(unsigned rings, unsigned p)
{
    uint64_t count = 0;
    unsigned i;

    for (i = 0; i <= p; ++i)
        count += test_g4(rings, i) * test_g4(rings, p - i);
    return count;
}

static uint64_t rank_pair_prefix(unsigned rings, unsigned total,
                                 unsigned first)
{
    uint64_t rank = 0;
    unsigned i;

    for (i = 0; i < first; ++i)
        rank += test_g2(rings, i) * test_g2(rings, total - i);
    return rank;
}

static uint64_t shell_rank(const uint8_t r[8], unsigned rings)
{
    unsigned a = r[0] + r[1] + r[2] + r[3] +
                 r[4] + r[5] + r[6] + r[7];
    unsigned b = r[0] + r[1] + r[2] + r[3];
    unsigned c = r[0] + r[1];
    unsigned d = r[4] + r[5];
    unsigned e = c < rings ? r[0] : rings - 1u - r[1];
    unsigned f = b - c < rings ? r[2] : rings - 1u - r[3];
    unsigned g = d < rings ? r[4] : rings - 1u - r[5];
    unsigned h = a - b - d < rings ? r[6] : rings - 1u - r[7];
    uint64_t r4 = (uint64_t)f * test_g2(rings, c) + e;
    uint64_t r5 = (uint64_t)h * test_g2(rings, d) + g;
    uint64_t r2 = rank_pair_prefix(rings, b, c) + r4;
    uint64_t r3 = rank_pair_prefix(rings, a - b, d) + r5;
    uint64_t rank = r3 * test_g4(rings, b) + r2;
    unsigned i;

    for (i = 0; i < b; ++i)
        rank += test_g4(rings, i) * test_g4(rings, a - i);
    for (i = 0; i < a; ++i)
        rank += test_g8(rings, i);
    return rank;
}

static void check_shell_mapper(void)
{
    v34_mapping_parameters p;
    uint8_t bits[V34_MAX_SHELL_BITS] = {0};
    uint8_t rings[8];
    unsigned value;
    unsigned bit;
    unsigned i;

    assert(v34_mapping_parameters_init(&p, 8));
    assert(v34_shell_map(&p, NULL, false, rings));
    for (i = 0; i < 8; ++i)
        assert(rings[i] == 0);

    /* K=12 at b=24: exhaustively require a distinct valid tuple per rank. */
    assert(v34_mapping_parameters_init(&p, 24));
    assert(p.shell_bits == 12 && p.minimum_rings == 3);
    for (value = 0; value < 4096; ++value) {
        for (bit = 0; bit < p.shell_bits; ++bit)
            bits[bit] = (uint8_t)((value >> bit) & 1u);
        assert(v34_shell_map(&p, bits, false, rings));
        for (i = 0; i < 8; ++i)
            assert(rings[i] < p.minimum_rings);
        assert(shell_rank(rings, p.minimum_rings) == value);
    }

    assert(v34_mapping_parameters_init(&p, 79));
    memset(bits, 1, p.shell_bits);
    assert(v34_shell_map(&p, bits, false, rings));
    for (i = 0; i < 8; ++i)
        assert(rings[i] < p.minimum_rings);
    assert(v34_shell_map(&p, bits, true, rings));
    for (i = 0; i < 8; ++i)
        assert(rings[i] < p.expanded_rings);
}

int main(void)
{
    check_parameters();
    check_large_parser();
    check_small_parser();
    check_shell_mapper();
    puts("v34 mapping parameters and parser tests: ok");
    return 0;
}
