#include "v34_phase4.h"
#include "v34_training_rx.h"
#include "v34_training_tx.h"
#include "v34_caps.h"
#include "v34_mp_receiver.h"

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
    v34_scrambler mp_seed;
    v34_mp_receiver mp_rx;
    v34_mp0 decoded_mp = {0}, decoded_prime = {0};
    v34_final_rates final_rates;
    unsigned trn_rotation = 0;
    bool mp_ready = false, have_mp = false, have_prime = false;

    v34_scrambler_init(&phase3, true);
    v34_scrambler_init(&mp_seed, true);
    for (unsigned n = 0; n < 512u; ++n) {
        assert(v34_trn4_phase(&mp_seed, &received));
        trn_rotation = ((12u - received) % 12u) / 3u;
    }
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
        if (symbols == 520u) {
            assert(v34_mp_receiver_init(&mp_rx, &mp_seed, trn_rotation));
            mp_ready = true;
        } else if (mp_ready && symbols > 520u && symbols <= 564u) {
            if (v34_mp_receiver_feed(&mp_rx, received, &decoded_mp)) {
                assert(symbols == 564u);
                have_mp = true;
                v34_mp_receiver_next(&mp_rx);
            }
        } else if (mp_ready && symbols > 564u && symbols <= 608u) {
            if (v34_mp_receiver_feed(&mp_rx, received, &decoded_prime)) {
                assert(symbols == 608u);
                have_prime = true;
            }
        }
        if (symbols < 618u) {
            assert(v34_phase4_next(&phase4, &expected));
            v34_training_tx_set_phase(&tx, expected);
        }
    }
    assert(symbols == 618u);
    assert(!v34_phase4_next(&phase4, &expected));
    assert(phase4.state == V34_P4_COMPLETE);
    assert(have_mp && have_prime);
    assert(!decoded_mp.acknowledge && decoded_prime.acknowledge);
    assert(v34_mp0_negotiate_rates(&mp, &decoded_prime, 33600,
                                   &final_rates));
    assert(final_rates.call_to_answer == 33600u);
    assert(final_rates.answer_to_call == 28800u);
}

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate);
    puts("v34 Phase 4 QAM/PCMA link: all six symbol rates pass");
    return 0;
}
