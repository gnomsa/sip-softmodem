#include "v34_b1.h"
#include "v34_trellis.h"

#include <assert.h>
#include <stdio.h>

static uint8_t bit(uint8_t value, unsigned position)
{
    return (uint8_t)((value >> position) & 1u);
}

static uint8_t reference_next(v34_trellis_kind kind, uint8_t state,
                              uint8_t y4, uint8_t y3,
                              uint8_t y2, uint8_t y1)
{
    uint8_t d0 = bit(state, 0);
    uint8_t d1 = bit(state, 1);
    uint8_t d2 = bit(state, 2);
    uint8_t d3 = bit(state, 3);
    uint8_t next = 0;

    if (kind == V34_TRELLIS_16) {
        next = d3 | ((d0 ^ y2) << 1u) | ((d1 ^ y2) << 2u) |
               ((d2 ^ y1) << 3u);
    } else if (kind == V34_TRELLIS_32) {
        uint8_t d4 = bit(state, 4);
        next = d4 | ((d0 ^ y2) << 1u) | ((d1 ^ y1) << 2u) |
               ((d2 ^ y4) << 3u) | ((d3 ^ y2) << 4u);
    } else {
        uint8_t d4 = bit(state, 4);
        uint8_t d5 = bit(state, 5);
        uint8_t a = d0 ^ d1;
        uint8_t b = d1 ^ y1;
        next = (y4 ^ a ^ (d2 & b)) |
               ((a ^ d3 ^ y3 ^ (y2 & d2)) << 1u) |
               ((b ^ d2) << 2u) | (d2 << 3u) | (d5 << 4u) |
               ((d4 ^ d2 ^ y2) << 5u);
    }
    return next;
}

static void check_transitions(v34_trellis_kind kind)
{
    unsigned states = 16u << (unsigned)kind;
    unsigned state;
    unsigned input;

    for (state = 0; state < states; ++state) {
        for (input = 0; input < 16; ++input) {
            v34_trellis_encoder encoder;
            uint8_t y4 = bit((uint8_t)input, 3);
            uint8_t y3 = bit((uint8_t)input, 2);
            uint8_t y2 = bit((uint8_t)input, 1);
            uint8_t y1 = bit((uint8_t)input, 0);
            uint8_t expected_output = bit((uint8_t)state,
                                          3u + (unsigned)kind);

            assert(v34_trellis_init(&encoder, kind));
            encoder.state = (uint8_t)state;
            assert(v34_trellis_output(&encoder) == expected_output);
            assert(v34_trellis_put(&encoder, y4, y3, y2, y1) ==
                   expected_output);
            assert(encoder.state ==
                   reference_next(kind, (uint8_t)state, y4, y3, y2, y1));
        }
    }
}

static void check_reachability(v34_trellis_kind kind)
{
    bool reached[64] = {false};
    unsigned states = 16u << (unsigned)kind;
    unsigned pass;
    unsigned state;
    unsigned input;

    reached[0] = true;
    for (pass = 0; pass < states; ++pass) {
        bool next[64];
        for (state = 0; state < states; ++state)
            next[state] = reached[state];
        for (state = 0; state < states; ++state) {
            if (!reached[state])
                continue;
            for (input = 0; input < 16; ++input)
                next[reference_next(kind, (uint8_t)state,
                                    bit((uint8_t)input, 3),
                                    bit((uint8_t)input, 2),
                                    bit((uint8_t)input, 1),
                                    bit((uint8_t)input, 0))] = true;
        }
        for (state = 0; state < states; ++state)
            reached[state] = next[state];
    }
    for (state = 0; state < states; ++state)
        assert(reached[state]);
}

static void check_subset_tables(void)
{
    static const uint8_t labels[4][4] = {
        {1,6,5,2}, {4,3,0,7}, {5,2,1,6}, {0,7,4,3}
    };
    static const int coordinates[4] = {-3,-1,1,3};
    static const int heights[4] = {3,1,-1,-3};
    unsigned row;
    unsigned column;

    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            v34_point point = {
                (int16_t)coordinates[column], (int16_t)heights[row]
            };
            uint8_t label;
            assert(v34_subset_label(point, &label));
            assert(label == labels[row][column]);
        }
    }
    {
        uint8_t y4, y3, y2, y1;
        assert(v34_subset_pair_bits(1, 0, &y4, &y3, &y2, &y1));
        assert(!y4 && !y3 && y2 && y1); /* Table 13: 0011. */
        assert(v34_subset_pair_bits(7, 6, &y4, &y3, &y2, &y1));
        assert(!y4 && y3 && y2 && y1); /* Table 13: 0111. */
    }
}

static void check_modulo(void)
{
    v34_point zero = {0,0};
    v34_point even = {2,2};
    v34_point odd = {2,0};

    assert(v34_modulo_bit(zero, even) == 0);
    assert(v34_modulo_bit(zero, odd) == 1);
    assert(v34_modulo_bit(odd, odd) == 0);
}

static void check_b1(v34_trellis_kind kind)
{
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    v34_mapping_parameters parameters;
    v34_parsed_mapping_frame parsed;
    v34_mapped_frame mapped;
    v34_b1_frame b1;
    unsigned frame;
    unsigned j;

    v34_constellation_init(&constellation);
    v34_differential_init(&differential);
    assert(v34_trellis_init(&trellis, kind));
    assert(v34_b1_frame_init(&b1, V34_SYMBOL_3429, 33600, true));
    assert(v34_mapping_parameters_init(&parameters,
                                       b1.geometry.high_frame_bits));
    for (frame = 0; frame < b1.geometry.mapping_frames_per_data_frame;
         ++frame) {
        assert(v34_parse_mapping_frame(&parameters,
                                       v34_b1_mapping_data(&b1, frame),
                                       v34_b1_mapping_bits(&b1, frame),
                                       &parsed));
        assert(v34_map_parsed_frame(&parameters, &parsed, true,
                                    &constellation, &differential, &mapped));
        for (j = 0; j < 4; ++j) {
            v34_point output[2];
            uint8_t v0 = (uint8_t)v34_b1_v0(&b1, 4u * frame + j);
            assert(v34_encode_4d_zero_precoder(&trellis,
                       mapped.quarter_point[j][0],
                       mapped.quarter_point[j][1], mapped.z[j],
                       mapped.i1[j], v0, output));
            assert((output[0].re & 1) && (output[0].im & 1));
            assert((output[1].re & 1) && (output[1].im & 1));
        }
    }
}

int main(void)
{
    check_transitions(V34_TRELLIS_16);
    check_transitions(V34_TRELLIS_32);
    check_transitions(V34_TRELLIS_64);
    check_reachability(V34_TRELLIS_16);
    check_reachability(V34_TRELLIS_32);
    check_reachability(V34_TRELLIS_64);
    check_subset_tables();
    check_modulo();
    check_b1(V34_TRELLIS_16);
    check_b1(V34_TRELLIS_32);
    check_b1(V34_TRELLIS_64);
    puts("v34 16/32/64-state trellis and B1 4D mapping tests: ok");
    return 0;
}
