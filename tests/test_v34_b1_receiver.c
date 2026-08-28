#include "v34_b1_receiver.h"
#include "v34_b1_stream.h"

#include <assert.h>
#include <stdio.h>

static const unsigned maximum_rate[V34_SYMBOL_COUNT] = {
    21600, 26400, 26400, 28800, 31200, 33600
};

static void run(v34_symbol_rate rate, v34_trellis_kind trellis,
                bool expanded, bool call_modem)
{
    const double scale = 180.0;
    v34_b1_stream tx;
    v34_b1_receiver rx;

    assert(v34_b1_stream_init(&tx, rate, maximum_rate[rate], call_modem,
                              trellis, expanded, call_modem, 8000, scale));
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
}

int main(void)
{
    unsigned rate;

    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate) {
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false, true);
        run((v34_symbol_rate)rate, V34_TRELLIS_32, false, false);
    }
    run(V34_SYMBOL_3429, V34_TRELLIS_16, false, true);
    run(V34_SYMBOL_3429, V34_TRELLIS_64, true, true);
    puts("v34 autonomous B1 PCMA receiver: all symbol rates pass");
    return 0;
}
