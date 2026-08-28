#include "v34_data_stream.h"

#include "v34_b1_receiver.h"
#include "v34_data_receiver.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const unsigned maximum_rate[V34_SYMBOL_COUNT] = {
    21600, 26400, 26400, 28800, 31200, 33600
};

static void run(v34_symbol_rate rate, v34_trellis_kind trellis,
                bool expanded, double phase_offset, double frequency_offset)
{
    const double scale = 180.0;
    v34_b1_stream b1;
    v34_b1_receiver b1_receiver;
    v34_data_stream data;
    v34_data_receiver receiver;
    v34_scrambler expected_scrambler;
    uint8_t input[V34_MAX_SUPERFRAME_BITS];
    uint8_t output[V34_MAX_SUPERFRAME_BITS];
    size_t input_count;
    size_t output_count = 0;
    size_t i;

    assert(v34_b1_stream_init(&b1, rate, maximum_rate[rate], true,
                              trellis, expanded, true, 8000, scale));
    b1.tx.carrier_phase += phase_offset;
    b1.tx.carrier_step += frequency_offset;
    assert(v34_b1_receiver_init(&b1_receiver, rate, maximum_rate[rate],
                                true, trellis, expanded, true, 8000, scale));

    /* Keep the receiver carrier and fractional clock continuous through B1. */
    while (!v34_b1_stream_complete(&b1)) {
        uint8_t sample;
        assert(v34_b1_stream_generate(&b1, &sample, 1) == 1);
        assert(v34_b1_receiver_feed(&b1_receiver, sample));
    }
    assert(v34_b1_receiver_complete(&b1_receiver));

    input_count = (size_t)b1.b1.geometry.bits_per_data_frame *
                  b1.b1.geometry.data_frames_per_superframe;
    assert(input_count <= sizeof(input));
    for (i = 0; i < input_count; ++i)
        input[i] = (uint8_t)(((i * 13u) ^ (i >> 2u) ^ (i >> 5u)) & 1u);

    expected_scrambler = b1.b1.scrambler_after;
    assert(v34_data_stream_init_after_b1(&data, &b1, input, input_count));
    assert(data.frame_bits[0] ==
           v34_scramble_bit(&expected_scrambler, input[0]));
    assert(v34_data_receiver_init_after_b1(&receiver, &b1_receiver));

    while (!v34_data_stream_complete(&data)) {
        uint8_t packet[160];

        assert(v34_data_stream_generate(&data, packet, sizeof(packet)) ==
               sizeof(packet));
        assert(v34_data_receiver_feed(&receiver, packet, sizeof(packet)));
        output_count += v34_data_receiver_read(
            &receiver, output + output_count, sizeof(output) - output_count);
    }

    assert(v34_data_receiver_complete(&receiver));
    assert(data.input_offset == input_count);
    assert(data.symbols ==
           (uint64_t)b1.b1.geometry.data_frames_per_superframe *
           8u * b1.b1.geometry.mapping_frames_per_data_frame);
    assert(receiver.symbols == data.symbols);
    assert(output_count == input_count);
    assert(memcmp(output, input, input_count) == 0);
    assert(v34_data_receiver_read(&receiver, output, sizeof(output)) == 0u);
    assert(data.active_samples == 2240u); /* One V.34 superframe is 280 ms. */
    assert(fabs(receiver.rx.carrier_step - data.tx.carrier_step) < 2e-5);
}

int main(void)
{
    unsigned rate;

    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false, 0.0, 0.0);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, 0.0, 0.0);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, 0.55, 3e-4);
    puts("v34 autonomous data receiver: 14 PCMA/RTP blocks pass");
    return 0;
}
