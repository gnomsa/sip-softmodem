#include "v34_b1_receiver.h"
#include "v34_b1_stream.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const unsigned maximum_rate[V34_SYMBOL_COUNT] = {
    21600, 26400, 26400, 28800, 31200, 33600
};

static void run(v34_symbol_rate rate, v34_trellis_kind trellis,
                bool expanded, bool call_modem,
                double phase_offset, double frequency_offset)
{
    const double scale = 180.0;
    v34_b1_stream tx;
    v34_b1_receiver rx;

    assert(v34_b1_stream_init(&tx, rate, maximum_rate[rate], call_modem,
                              trellis, expanded, call_modem, 8000, scale));
    tx.tx.carrier_phase += phase_offset;
    tx.tx.carrier_step += frequency_offset;
    assert(v34_b1_receiver_init(&rx, rate, maximum_rate[rate], call_modem,
                                trellis, expanded, call_modem, 8000, scale));
    while (!v34_b1_stream_complete(&tx)) {
        uint8_t sample;
        assert(v34_b1_stream_generate(&tx, &sample, 1) == 1);
        assert(v34_b1_receiver_feed(&rx, sample));
    }
    assert(v34_b1_receiver_complete(&rx));
    assert(rx.received_bits == tx.b1.geometry.bits_per_data_frame);
    assert(rx.bit_errors == 0);
    assert(rx.decoder.trellis.state == tx.trellis.state);
    assert(rx.decoder.differential.previous_rotation ==
           tx.differential.previous_rotation);
    assert(rx.descrambler.history == tx.b1.scrambler_after.history);
    assert(rx.descrambler.tap == tx.b1.scrambler_after.tap);
    assert(fabs(rx.rx.carrier_step - tx.tx.carrier_step) < 2e-5);
}

int main(void)
{
    unsigned rate;

    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate) {
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false, true, 0.0, 0.0);
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false, false, 0.0, 0.0);
    }
    run(V34_SYMBOL_3429, V34_TRELLIS_16, false, true, 0.0, 0.0);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, true, 0.0, 0.0);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, true, 0.55, 3e-4);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, false, -0.65, -2.5e-4);
    puts("v34 autonomous B1 PCMA receiver: carrier offsets pass");
    return 0;
}
