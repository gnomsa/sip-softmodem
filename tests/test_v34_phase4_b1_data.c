#include "v34_b1_data_link.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    v34_scrambler call_phase3, answer_phase3;
    v34_phase4_stream call_p4_tx, answer_p4_tx;
    v34_phase4_receiver call_p4_rx, answer_p4_rx;
    v34_b1_data_link caller, answerer;
    v34_mp0 call_mp = {14, 12, false, 2, false, true, false,
                       V34_RATE_ALL_MASK, true};
    v34_mp0 answer_mp = {14, 13, false, 2, false, true, false,
                         V34_RATE_ALL_MASK, true};
    v34_final_rates rates;
    uint8_t call_input[1403], answer_input[997];
    uint8_t call_output[1403], answer_output[997];
    size_t call_count = 0, answer_count = 0, i;
    unsigned packet;

    v34_scrambler_init(&call_phase3, true);
    v34_scrambler_init(&answer_phase3, false);
    assert(v34_phase4_stream_init(
        &call_p4_tx, true, &call_phase3, 0u, &call_mp,
        V34_SYMBOL_3429, true, 8000, 9000.0));
    assert(v34_phase4_stream_init(
        &answer_p4_tx, false, &answer_phase3, 0u, &answer_mp,
        V34_SYMBOL_3429, false, 8000, 9000.0));
    assert(v34_phase4_receiver_init(
        &call_p4_rx, false, &answer_phase3, 0u,
        V34_SYMBOL_3429, false, 8000));
    assert(v34_phase4_receiver_init(
        &answer_p4_rx, true, &call_phase3, 0u,
        V34_SYMBOL_3429, true, 8000));

    while (!v34_phase4_stream_complete(&call_p4_tx) ||
           !v34_phase4_stream_complete(&answer_p4_tx)) {
        uint8_t call_pcma[160], answer_pcma[160];
        size_t sample;
        assert(v34_phase4_stream_generate(
            &call_p4_tx, call_pcma, sizeof(call_pcma)) == sizeof(call_pcma));
        assert(v34_phase4_stream_generate(
            &answer_p4_tx, answer_pcma, sizeof(answer_pcma)) ==
            sizeof(answer_pcma));
        for (sample = 0; sample < 160u; ++sample) {
            assert(v34_phase4_receiver_feed(&call_p4_rx,
                                             answer_pcma[sample]));
            assert(v34_phase4_receiver_feed(&answer_p4_rx,
                                             call_pcma[sample]));
        }
    }
    assert(v34_mp0_negotiate_rates(
        &call_mp, &call_p4_rx.mp_prime, 33600u, &rates));
    assert(rates.call_to_answer == 33600u);
    assert(rates.answer_to_call == 28800u);

    assert(v34_b1_data_link_init_after_phase4(
        &caller, true, &call_p4_tx, &call_p4_rx,
        V34_SYMBOL_3429, rates.call_to_answer,
        V34_SYMBOL_3429, rates.answer_to_call,
        V34_TRELLIS_64, true, 8000, 180.0));
    assert(v34_b1_data_link_init_after_phase4(
        &answerer, false, &answer_p4_tx, &answer_p4_rx,
        V34_SYMBOL_3429, rates.answer_to_call,
        V34_SYMBOL_3429, rates.call_to_answer,
        V34_TRELLIS_64, true, 8000, 180.0));
    assert(fabs(caller.tx_b1.tx.carrier_phase -
                call_p4_tx.tx.carrier_phase) < 1e-12);
    assert(caller.tx_b1.clock.phase == call_p4_tx.clock.phase);
    assert(caller.rx_b1.rx.clock.phase == call_p4_rx.rx.clock.phase);

    for (i = 0; i < sizeof(call_input); ++i)
        call_input[i] = (uint8_t)((i * 47u) ^ (i >> 1u));
    for (i = 0; i < sizeof(answer_input); ++i)
        answer_input[i] = (uint8_t)((i * 89u) ^ (i >> 3u));
    assert(v34_b1_data_link_write(
        &caller, call_input, sizeof(call_input)) == sizeof(call_input));
    assert(v34_b1_data_link_write(
        &answerer, answer_input, sizeof(answer_input)) ==
        sizeof(answer_input));

    for (packet = 0; packet < 44u; ++packet) {
        uint8_t call_pcma[160], answer_pcma[160], bytes[53];
        size_t count;
        assert(v34_b1_data_link_generate(&caller, call_pcma));
        assert(v34_b1_data_link_generate(&answerer, answer_pcma));
        assert(v34_b1_data_link_receive(&caller, answer_pcma));
        assert(v34_b1_data_link_receive(&answerer, call_pcma));
        while ((count = v34_b1_data_link_read(
                    &answerer, bytes, sizeof(bytes))) != 0u) {
            assert(call_count + count <= sizeof(call_output));
            memcpy(call_output + call_count, bytes, count);
            call_count += count;
        }
        while ((count = v34_b1_data_link_read(
                    &caller, bytes, sizeof(bytes))) != 0u) {
            assert(answer_count + count <= sizeof(answer_output));
            memcpy(answer_output + answer_count, bytes, count);
            answer_count += count;
        }
    }
    assert(call_count == sizeof(call_input));
    assert(answer_count == sizeof(answer_input));
    assert(memcmp(call_output, call_input, sizeof(call_input)) == 0);
    assert(memcmp(answer_output, answer_input, sizeof(answer_input)) == 0);
    printf("v34 E-to-B1/data: 33600/28800, %zu + %zu exact bytes\n",
           call_count, answer_count);
    return 0;
}
