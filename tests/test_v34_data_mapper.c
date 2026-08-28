#include "v34_b1.h"
#include "v34_data_mapper.h"

#include <assert.h>
#include <stdio.h>

static void expect_point(const v34_constellation *c, unsigned index,
                         int re, int im)
{
    v34_point point;

    assert(v34_constellation_point(c, index, &point));
    assert(point.re == re && point.im == im);
}

static void check_constellation(void)
{
    v34_constellation c;
    v34_point point;
    v34_point rotated;
    unsigned i;
    unsigned previous_energy = 0;

    v34_constellation_init(&c);
    expect_point(&c, 0, 1, 1);
    expect_point(&c, 1, -3, 1);
    expect_point(&c, 2, 1, -3);
    expect_point(&c, 3, -3, -3);
    expect_point(&c, 4, 1, 5);
    expect_point(&c, 5, 5, 1);
    expect_point(&c, 394, 1, 45);
    expect_point(&c, 396, -3, 45);
    expect_point(&c, 400, 5, 45);
    expect_point(&c, 408, -7, 45);
    expect_point(&c, 414, 9, 45);

    for (i = 0; i < V34_QUARTER_POINTS; ++i) {
        unsigned energy;
        assert(v34_constellation_point(&c, i, &point));
        assert((point.re - 1) % 4 == 0 && (point.im - 1) % 4 == 0);
        energy = (unsigned)(point.re * point.re + point.im * point.im);
        assert(energy >= previous_energy);
        previous_energy = energy;
    }
    point.re = 1;
    point.im = 5;
    rotated = v34_rotate_clockwise(point, 1);
    assert(rotated.re == 5 && rotated.im == -1);
    rotated = v34_rotate_clockwise(point, 3);
    assert(rotated.re == -5 && rotated.im == 1);
}

static void check_differential(void)
{
    static const uint8_t input[6][2] = {
        {0,0}, {1,0}, {0,1}, {1,1}, {1,0}, {0,0}
    };
    static const uint8_t expected[6] = {0,1,3,2,3,3};
    v34_differential_encoder encoder;
    unsigned i;

    v34_differential_init(&encoder);
    for (i = 0; i < 6; ++i)
        assert(v34_differential_put(&encoder, input[i][0], input[i][1]) ==
               expected[i]);
}

static unsigned q_value(const v34_parsed_mapping_frame *parsed,
                        unsigned j, unsigned symbol)
{
    unsigned value = 0;
    unsigned bit;

    for (bit = 0; bit < parsed->q_bit_count; ++bit)
        value |= (unsigned)parsed->q[j][symbol][bit] << bit;
    return value;
}

static void check_b1_mapping(v34_symbol_rate rate, unsigned data_rate,
                             bool expanded)
{
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_mapping_parameters parameters;
    v34_parsed_mapping_frame parsed;
    v34_mapped_frame mapped;
    v34_b1_frame b1;
    unsigned frame;
    unsigned j;
    unsigned symbol;

    v34_constellation_init(&constellation);
    v34_differential_init(&differential);
    assert(v34_b1_frame_init(&b1, rate, data_rate, true));
    assert(v34_mapping_parameters_init(&parameters,
                                       b1.geometry.high_frame_bits));
    for (frame = 0; frame < b1.geometry.mapping_frames_per_data_frame;
         ++frame) {
        assert(v34_parse_mapping_frame(&parameters,
                                       v34_b1_mapping_data(&b1, frame),
                                       v34_b1_mapping_bits(&b1, frame),
                                       &parsed));
        assert(v34_map_parsed_frame(&parameters, &parsed, expanded,
                                    &constellation, &differential, &mapped));
        for (j = 0; j < 4; ++j) {
            assert(mapped.z[j] < 4 && mapped.i1[j] < 2);
            for (symbol = 0; symbol < 2; ++symbol) {
                assert(mapped.q_index[j][symbol] < V34_QUARTER_POINTS);
                assert(mapped.q_index[j][symbol] ==
                       ((unsigned)mapped.rings[j][symbol] <<
                        parameters.q_bits) + q_value(&parsed, j, symbol));
            }
        }
    }
}

int main(void)
{
    check_constellation();
    check_differential();
    check_b1_mapping(V34_SYMBOL_2400, 2400, false);
    check_b1_mapping(V34_SYMBOL_3000, 19200, false);
    check_b1_mapping(V34_SYMBOL_3429, 33600, true);
    puts("v34 differential and quarter-constellation mapping tests: ok");
    return 0;
}
