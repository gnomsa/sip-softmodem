#include "v34_b1_stream.h"
#include "v34_training_rx.h"

#include "pcma.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const unsigned maximum_rate[V34_SYMBOL_COUNT] = {
    21600, 26400, 26400, 28800, 31200, 33600
};

static void run(v34_symbol_rate rate, v34_trellis_kind trellis,
                bool expanded)
{
    v34_b1_stream stream;
    uint8_t packet[160];
    uint64_t expected_symbols;
    uint64_t expected_samples;
    uint64_t packets = 0;
    long long energy = 0;
    unsigned i;

    assert(v34_b1_stream_init(&stream, rate, maximum_rate[rate], true,
                              trellis, expanded, true, 8000, 180.0));
    expected_symbols =
        8u * stream.b1.geometry.mapping_frames_per_data_frame;
    expected_samples = v34_samples_for_symbols(rate, 8000,
                                                expected_symbols);
    while (!v34_b1_stream_complete(&stream) && packets < 100u) {
        size_t generated = v34_b1_stream_generate(&stream, packet,
                                                   sizeof(packet));
        if (generated != sizeof(packet))
            fprintf(stderr, "rate=%u trellis=%u frame=%u interval=%u generated=%zu\n",
                    rate, trellis, stream.mapping_frame,
                    stream.interval_4d, generated);
        assert(generated == sizeof(packet));
        for (i = 0; i < sizeof(packet); ++i) {
            int sample = pcma_decode(packet[i]);
            energy += (long long)sample * sample;
        }
        packets++;
    }
    assert(v34_b1_stream_complete(&stream));
    assert(v34_b1_stream_symbols(&stream) == expected_symbols);
    assert(stream.active_samples == expected_samples);
    assert(packets * sizeof(packet) >= expected_samples);
    assert(packets * sizeof(packet) - expected_samples < sizeof(packet));
    assert(energy > 100000000LL);

    assert(v34_b1_stream_generate(&stream, packet, sizeof(packet)) ==
           sizeof(packet));
    for (i = 0; i < sizeof(packet); ++i)
        assert(pcma_decode(packet[i]) == 8);
}

static void check_qam_sample(void)
{
    v34_qam_tx tx;
    v34_point point = {3, 5};
    int sample;

    assert(v34_qam_tx_init(&tx, V34_SYMBOL_2400, false, 8000, 100.0));
    v34_qam_tx_set_point(&tx, point);
    sample = v34_qam_tx_sample(&tx);
    assert(sample == 300);
}

static void check_pcma_iq_loopback(v34_symbol_rate rate,
                                   v34_trellis_kind trellis,
                                   bool expanded)
{
    const double scale = 180.0;
    v34_b1_stream stream;
    v34_training_rx rx;
    unsigned decoded_symbols = 0;

    assert(v34_b1_stream_init(&stream, rate, maximum_rate[rate], false,
                              trellis, expanded, false, 8000, scale));
    assert(v34_training_rx_init(&rx, rate, false, 8000));
    while (!v34_b1_stream_complete(&stream)) {
        v34_point expected = stream.tx.point;
        uint8_t sample;
        double in_phase;
        double quadrature;

        assert(v34_b1_stream_generate(&stream, &sample, 1) == 1);
        if (!v34_training_rx_pcma_iq(&rx, sample,
                                     &in_phase, &quadrature))
            continue;
        assert(fabs(in_phase / scale - expected.re) < 1.0);
        assert(fabs(quadrature / scale - expected.im) < 1.0);
        decoded_symbols++;
    }
    assert(decoded_symbols == v34_b1_stream_symbols(&stream));
}

int main(void)
{
    unsigned rate;

    check_qam_sample();
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false);
    run(V34_SYMBOL_3429, V34_TRELLIS_16, false);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true);
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        check_pcma_iq_loopback((v34_symbol_rate)rate,
                               V34_TRELLIS_32, false);
    check_pcma_iq_loopback(V34_SYMBOL_3429, V34_TRELLIS_64, true);
    puts("v34 B1 160-byte PCMA stream: all symbol rates and trellises pass");
    return 0;
}
