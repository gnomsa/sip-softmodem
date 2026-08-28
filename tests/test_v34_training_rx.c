#include "v34_training_rx.h"
#include "v34_training_tx.h"

#include <assert.h>
#include <stdio.h>

static void run(v34_symbol_rate rate)
{
    v34_training_tx tx;
    v34_training_rx rx;
    unsigned symbols = 0, samples;
    uint8_t expected = 0, got;
    double magnitude;
    assert(v34_training_tx_init(&tx, rate, true, 8000, 9000));
    assert(v34_training_rx_init(&rx, rate, true, 8000));
    v34_training_tx_set_phase(&tx, expected);
    for (samples = 0; symbols < 256 && samples < 10000; ++samples) {
        uint8_t encoded = v34_training_tx_pcma(&tx);
        if (v34_training_rx_pcma(&rx, encoded, &got, &magnitude)) {
            assert(got == expected);
            assert(magnitude > 7000.0);
            symbols++;
            expected = (uint8_t)((expected + 3u) % 12u);
            v34_training_tx_set_phase(&tx, expected);
        }
    }
    assert(symbols == 256);
}

int main(void)
{
    run(V34_SYMBOL_2400);
    run(V34_SYMBOL_3200);
    run(V34_SYMBOL_3429);
    puts("v34 coherent PCMA training receiver tests: ok");
    return 0;
}
