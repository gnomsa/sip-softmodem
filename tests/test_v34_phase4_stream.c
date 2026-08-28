#include "v34_phase4_receiver.h"
#include "v34_phase4_stream.h"

#include <assert.h>
#include <stdio.h>

static void run(v34_symbol_rate rate)
{
    v34_scrambler call_phase3, answer_phase3;
    v34_phase4_stream call_tx, answer_tx;
    v34_phase4_receiver call_rx, answer_rx;
    v34_mp0 call_mp = {14, 12, false, 2, false, true, false,
                       V34_RATE_ALL_MASK, true};
    v34_mp0 answer_mp = {14, 13, false, 2, false, true, false,
                         V34_RATE_ALL_MASK, true};
    unsigned packets = 0;

    v34_scrambler_init(&call_phase3, true);
    v34_scrambler_init(&answer_phase3, false);
    assert(v34_phase4_stream_init(
        &call_tx, true, &call_phase3, 0u, &call_mp,
        rate, true, 8000, 9000.0));
    assert(v34_phase4_stream_init(
        &answer_tx, false, &answer_phase3, 0u, &answer_mp,
        rate, false, 8000, 9000.0));
    assert(v34_phase4_receiver_init(
        &call_rx, false, &answer_phase3, 0u,
        rate, false, 8000));
    assert(v34_phase4_receiver_init(
        &answer_rx, true, &call_phase3, 0u,
        rate, true, 8000));

    while (!v34_phase4_stream_complete(&call_tx) ||
           !v34_phase4_stream_complete(&answer_tx)) {
        uint8_t call_pcma[160], answer_pcma[160];
        size_t sample;

        assert(v34_phase4_stream_generate(
            &call_tx, call_pcma, sizeof(call_pcma)) == sizeof(call_pcma));
        assert(v34_phase4_stream_generate(
            &answer_tx, answer_pcma, sizeof(answer_pcma)) ==
            sizeof(answer_pcma));
        for (sample = 0; sample < sizeof(call_pcma); ++sample) {
            assert(v34_phase4_receiver_feed(&call_rx, answer_pcma[sample]));
            assert(v34_phase4_receiver_feed(&answer_rx, call_pcma[sample]));
        }
        packets++;
        assert(packets < 20u);
    }
    assert(v34_phase4_receiver_complete(&call_rx));
    assert(v34_phase4_receiver_complete(&answer_rx));
    assert(call_rx.mp_prime.call_to_answer_rate_2400 ==
           answer_mp.call_to_answer_rate_2400);
    assert(answer_rx.mp_prime.answer_to_call_rate_2400 ==
           call_mp.answer_to_call_rate_2400);
    assert(call_tx.symbols == 618u && answer_tx.symbols == 618u);
}

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate);
    puts("v34 autonomous Phase 4: MP/MP-prime/E on all rates pass");
    return 0;
}
