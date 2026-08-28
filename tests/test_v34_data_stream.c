#include "v34_data_stream.h"

#include "v34_training_rx.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const unsigned maximum_rate[V34_SYMBOL_COUNT] = {
    21600, 26400, 26400, 28800, 31200, 33600
};

static void run(v34_symbol_rate rate, v34_trellis_kind trellis,
                bool expanded)
{
    const double scale = 180.0;
    v34_b1_stream b1;
    v34_data_stream data;
    v34_training_rx rx;
    v34_scrambler expected_scrambler;
    uint8_t input[V34_MAX_SUPERFRAME_BITS];
    size_t input_count;
    size_t i;
    unsigned decoded = 0;

    assert(v34_b1_stream_init(&b1, rate, maximum_rate[rate], true,
                              trellis, expanded, true, 8000, scale));
    assert(v34_training_rx_init(&rx, rate, true, 8000));

    /* Keep the receiver carrier and fractional clock continuous through B1. */
    while (!v34_b1_stream_complete(&b1)) {
        uint8_t sample;
        double in_phase;
        double quadrature;
        assert(v34_b1_stream_generate(&b1, &sample, 1) == 1);
        (void)v34_training_rx_pcma_iq(&rx, sample,
                                      &in_phase, &quadrature);
    }

    input_count = (size_t)b1.b1.geometry.bits_per_data_frame *
                  b1.b1.geometry.data_frames_per_superframe;
    assert(input_count <= sizeof(input));
    for (i = 0; i < input_count; ++i)
        input[i] = (uint8_t)(((i * 13u) ^ (i >> 2u) ^ (i >> 5u)) & 1u);

    expected_scrambler = b1.b1.scrambler_after;
    assert(v34_data_stream_init_after_b1(&data, &b1, input, input_count));
    assert(data.frame_bits[0] ==
           v34_scramble_bit(&expected_scrambler, input[0]));

    while (!v34_data_stream_complete(&data)) {
        v34_point expected = data.tx.point;
        uint8_t sample;
        double in_phase;
        double quadrature;

        assert(v34_data_stream_generate(&data, &sample, 1) == 1);
        if (!v34_training_rx_pcma_iq(&rx, sample,
                                     &in_phase, &quadrature))
            continue;
        assert(fabs(in_phase / scale - expected.re) < 1.0);
        assert(fabs(quadrature / scale - expected.im) < 1.0);
        decoded++;
    }

    assert(data.input_offset == input_count);
    assert(data.symbols ==
           (uint64_t)b1.b1.geometry.data_frames_per_superframe *
           8u * b1.b1.geometry.mapping_frames_per_data_frame);
    assert(decoded == data.symbols);
    assert(data.active_samples == 2240u); /* One V.34 superframe is 280 ms. */
}

int main(void)
{
    unsigned rate;

    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true);
    puts("v34 B1-to-data PCMA superframe: all symbol rates pass");
    return 0;
}
