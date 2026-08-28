#include "v34_info.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_info0_round_trip(void)
{
    const v34_info0 input = {
        .symbol_2743 = true,
        .symbol_2800 = true,
        .symbol_3429 = true,
        .carrier_3000_low = true,
        .carrier_3000_high = false,
        .carrier_3200_low = true,
        .carrier_3200_high = true,
        .allow_3429 = true,
        .power_reduction = true,
        .maximum_symbol_rate_difference = 3,
        .cme = false,
        .constellation_1664 = true,
        .clock_source = V34_CLOCK_RECEIVE,
        .acknowledge = false,
    };
    uint8_t frame[V34_INFO0_BYTES];
    v34_info0 output;

    assert(v34_info0_encode(&input, frame));
    assert((frame[0] & 0x0fu) == 0x0fu);
    assert(((frame[0] >> 4) | ((frame[1] & 0x0fu) << 4)) == 0x4eu);
    assert(v34_info0_decode(frame, &output));
    assert(memcmp(&input, &output, sizeof(input)) == 0);

    frame[2] ^= 0x04u;
    assert(!v34_info0_decode(frame, &output));
}

static void test_validation(void)
{
    v34_info0 info = {0};
    uint8_t frame[V34_INFO0_BYTES];

    info.maximum_symbol_rate_difference = 6;
    assert(!v34_info0_encode(&info, frame));
    info.maximum_symbol_rate_difference = 0;
    info.clock_source = (v34_clock_source)3;
    assert(!v34_info0_encode(&info, frame));
}

int main(void)
{
    test_info0_round_trip();
    test_validation();
    puts("v34 INFO0 codec tests: ok");
    return 0;
}
