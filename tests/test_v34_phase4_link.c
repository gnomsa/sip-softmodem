#include "v34_phase4.h"
#include "v34_training_rx.h"
#include "v34_training_tx.h"
#include "v34_caps.h"

#include <assert.h>
#include <stdio.h>

static void run(v34_symbol_rate rate)
{
    v34_scrambler phase3;
    v34_phase4 phase4;
    v34_training_tx tx;
    v34_training_rx rx;
    v34_mp0 mp = {14, 12, false, 1, true, false, false,
                  V34_RATE_ALL_MASK, true};
    uint8_t expected, received;
    unsigned symbols = 0, samples = 0;

    v34_scrambler_init(&phase3, true);
    assert(v34_phase4_init(&phase4, true, &phase3, 0, &mp));
    assert(v34_training_tx_init(&tx, rate, true, 8000, 9000));
    assert(v34_training_rx_init(&rx, rate, true, 8000));
    assert(v34_phase4_next(&phase4, &expected));
    v34_training_tx_set_phase(&tx, expected);

    while (symbols < 618u && samples < 10000u) {
        uint8_t pcma = v34_training_tx_pcma(&tx);
        samples++;
        if (!v34_training_rx_pcma(&rx, pcma, &received, NULL))
            continue;
        assert(received == expected);
        symbols++;
        if (symbols < 618u) {
            assert(v34_phase4_next(&phase4, &expected));
            v34_training_tx_set_phase(&tx, expected);
        }
    }
    assert(symbols == 618u);
    assert(!v34_phase4_next(&phase4, &expected));
    assert(phase4.state == V34_P4_COMPLETE);
}

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate);
    puts("v34 Phase 4 QAM/PCMA link: all six symbol rates pass");
    return 0;
}
