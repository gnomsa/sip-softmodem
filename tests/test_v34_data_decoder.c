#include "v34_b1.h"
#include "v34_data_decoder.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void run(v34_symbol_rate rate, unsigned data_rate,
                v34_trellis_kind kind, bool expanded)
{
    v34_b1_frame b1;
    v34_mapping_parameters parameters;
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    v34_data_decoder decoder;
    unsigned mapping;

    assert(v34_b1_frame_init(&b1, rate, data_rate, true));
    assert(v34_mapping_parameters_init(&parameters,
                                       b1.geometry.high_frame_bits));
    v34_constellation_init(&constellation);
    v34_differential_init(&differential);
    assert(v34_trellis_init(&trellis, kind));
    assert(v34_data_decoder_init(&decoder, &parameters, kind, expanded));

    for (mapping = 0;
         mapping < b1.geometry.mapping_frames_per_data_frame;
         ++mapping) {
        v34_parsed_mapping_frame parsed;
        v34_mapped_frame mapped;
        v34_point received[4][2];
        uint8_t v0[4];
        uint8_t decoded[V34_MAX_DATA_FRAME_BITS];
        const uint8_t *expected = v34_b1_mapping_data(&b1, mapping);
        size_t expected_count = v34_b1_mapping_bits(&b1, mapping);
        size_t decoded_count = 0;
        unsigned j;

        assert(v34_parse_mapping_frame(&parameters, expected,
                                       expected_count, &parsed));
        assert(v34_map_parsed_frame(&parameters, &parsed, expanded,
                                    &constellation, &differential, &mapped));
        for (j = 0; j < 4; ++j) {
            v0[j] = (uint8_t)v34_b1_v0(&b1, 4u * mapping + j);
            assert(v34_encode_4d_zero_precoder(&trellis,
                       mapped.quarter_point[j][0],
                       mapped.quarter_point[j][1], mapped.z[j],
                       mapped.i1[j], v0[j], received[j]));
        }
        assert(v34_decode_mapping_frame(&decoder, received, v0,
                                        v34_mapping_frame_high(&b1.geometry,
                                                               mapping),
                                        decoded, &decoded_count));
        assert(decoded_count == expected_count);
        assert(memcmp(decoded, expected, expected_count) == 0);
        assert(decoder.trellis.state == trellis.state);
        assert(decoder.differential.previous_rotation ==
               differential.previous_rotation);
    }
}

int main(void)
{
    run(V34_SYMBOL_2400, 2400, V34_TRELLIS_16, false);
    run(V34_SYMBOL_3000, 19200, V34_TRELLIS_32, false);
    run(V34_SYMBOL_3200, 31200, V34_TRELLIS_64, false);
    run(V34_SYMBOL_3429, 33600, V34_TRELLIS_64, true);
    puts("v34 inverse shell and exact 4D mapping-frame decoder tests: ok");
    return 0;
}
