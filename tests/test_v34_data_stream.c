#include "v34_data_stream.h"

#include "v34_b1_receiver.h"
#include "v34_data_decoder.h"
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
    v34_b1_receiver b1_receiver;
    v34_data_stream data;
    v34_training_rx rx;
    v34_data_decoder decoder;
    v34_scrambler expected_scrambler;
    v34_scrambler descrambler;
    v34_point received[4][2];
    uint8_t input[V34_MAX_SUPERFRAME_BITS];
    size_t input_count;
    size_t i;
    unsigned decoded = 0;
    unsigned received_data_frame = 0;
    unsigned received_mapping_frame = 0;
    unsigned received_symbol = 0;
    size_t received_bits = 0;

    assert(v34_b1_stream_init(&b1, rate, maximum_rate[rate], true,
                              trellis, expanded, true, 8000, scale));
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
    decoder = b1_receiver.decoder;
    descrambler = b1_receiver.descrambler;
    rx = b1_receiver.rx;

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
        assert(v34_slice_iq(in_phase, quadrature, scale,
                            &received[received_symbol / 2u]
                                     [received_symbol % 2u]));
        received_symbol++;
        if (received_symbol == 8u) {
            uint8_t v0[4];
            uint8_t scrambled[V34_MAX_DATA_FRAME_BITS];
            size_t scrambled_count = 0;
            unsigned j;
            for (j = 0; j < 4u; ++j)
                v0[j] = (uint8_t)v34_sync_inversion(
                    &data.geometry, received_data_frame,
                    4u * received_mapping_frame + j);
            assert(v34_decode_mapping_frame(
                &decoder, received, v0,
                v34_mapping_frame_high(&data.geometry,
                                       received_mapping_frame),
                scrambled, &scrambled_count));
            for (i = 0; i < scrambled_count; ++i) {
                assert(received_bits < input_count);
                assert(v34_descramble_bit(&descrambler, scrambled[i]) ==
                       input[received_bits]);
                received_bits++;
            }
            received_symbol = 0;
            received_mapping_frame++;
            if (received_mapping_frame ==
                data.geometry.mapping_frames_per_data_frame) {
                received_mapping_frame = 0;
                received_data_frame++;
            }
        }
        decoded++;
    }

    assert(data.input_offset == input_count);
    assert(data.symbols ==
           (uint64_t)b1.b1.geometry.data_frames_per_superframe *
           8u * b1.b1.geometry.mapping_frames_per_data_frame);
    assert(decoded == data.symbols);
    assert(received_bits == input_count);
    assert(received_data_frame == data.geometry.data_frames_per_superframe);
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
