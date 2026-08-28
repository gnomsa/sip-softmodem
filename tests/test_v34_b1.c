#include "v34_b1.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_scrambler_prefixes(void)
{
    v34_scrambler scrambler;
    static const uint8_t call_expected[24] = {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1
    };
    static const uint8_t answer_expected[24] = {
        1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,0
    };
    unsigned i;

    v34_scrambler_init(&scrambler, true);
    for (i = 0; i < 24; ++i)
        assert(v34_scramble_bit(&scrambler, 1) == call_expected[i]);
    v34_scrambler_init(&scrambler, false);
    for (i = 0; i < 24; ++i)
        assert(v34_scramble_bit(&scrambler, 1) == answer_expected[i]);
}

static void check_frame(v34_symbol_rate symbol_rate, unsigned data_rate)
{
    v34_b1_frame call;
    v34_b1_frame answer;
    size_t total = 0;
    unsigned i;

    assert(v34_b1_frame_init(&call, symbol_rate, data_rate, true));
    assert(v34_b1_frame_init(&answer, symbol_rate, data_rate, false));
    for (i = 0; i < call.geometry.mapping_frames_per_data_frame; ++i) {
        size_t count = v34_b1_mapping_bits(&call, i);
        assert(count == call.geometry.high_frame_bits -
                        (v34_mapping_frame_high(&call.geometry, i) ? 0u : 1u));
        assert(v34_b1_mapping_data(&call, i) == call.bits + total);
        total += count;
    }
    assert(total == call.geometry.bits_per_data_frame);
    assert(memcmp(call.bits, answer.bits, total) != 0);
    assert(v34_b1_v0(&call, 0));
    assert(!v34_b1_v0(&call, 2u * call.geometry.mapping_frames_per_data_frame));
    assert(!v34_b1_v0(&call, 1));
}

static void check_sync_patterns(void)
{
    static const uint8_t j7[14] = {0,1,1,1,0,1,1,1,1,1,1,1,1,0};
    static const uint8_t j8[16] = {0,1,1,1,0,1,1,1,1,1,1,1,1,0,1,0};
    v34_frame_geometry geometry;
    unsigned frame;
    unsigned half;

    assert(v34_frame_geometry_init(&geometry, V34_SYMBOL_3000, 19200));
    for (frame = 0; frame < 7; ++frame)
        for (half = 0; half < 2; ++half)
            assert(v34_sync_inversion(&geometry, frame,
                       half * 2u * geometry.mapping_frames_per_data_frame) ==
                   (j7[2u * frame + half] != 0));

    assert(v34_frame_geometry_init(&geometry, V34_SYMBOL_3429, 33600));
    for (frame = 0; frame < 8; ++frame)
        for (half = 0; half < 2; ++half)
            assert(v34_sync_inversion(&geometry, frame,
                       half * 2u * geometry.mapping_frames_per_data_frame) ==
                   (j8[2u * frame + half] != 0));
}

int main(void)
{
    check_scrambler_prefixes();
    check_sync_patterns();
    check_frame(V34_SYMBOL_2400, 2400);
    check_frame(V34_SYMBOL_3000, 19200);
    check_frame(V34_SYMBOL_3429, 33600);
    puts("v34 B1 scrambling, framing, and V0 tests: ok");
    return 0;
}
