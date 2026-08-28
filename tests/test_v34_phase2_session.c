#include "v34_phase2_session.h"

#include <assert.h>
#include <stdio.h>

#define PACKET 160u
#define DELAY 10u

int main(void)
{
    const v34_info0 call_info = {
        true, true, true, true, true, true, true, true, false, 3,
        true, true, V34_CLOCK_INTERNAL, false
    };
    const v34_info0 answer_info = {
        true, true, true, true, true, true, true, true, false, 3,
        true, true, V34_CLOCK_RECEIVE, true
    };
    v34_phase2_session call, answer;
    uint8_t cq[DELAY][PACKET], aq[DELAY][PACKET];
    unsigned packet;

    assert(v34_phase2_session_init(&call, V34_INFO_CALL_MODEM, &call_info,
        V34_SYMBOL_ALL_MASK, V34_RATE_ALL_MASK, 33600u, 1u));
    assert(v34_phase2_session_init(&answer, V34_INFO_ANSWER_MODEM, &answer_info,
        V34_SYMBOL_ALL_MASK, V34_RATE_ALL_MASK, 33600u, 1u));

    for (packet = 0; packet < 500u &&
         (!v34_phase2_session_complete(&call) ||
          !v34_phase2_session_complete(&answer)); ++packet) {
        unsigned slot = packet % DELAY;
        if (packet >= DELAY) {
            unsigned old = (packet - DELAY) % DELAY;
            v34_phase2_session_receive(&call, aq[old], PACKET);
            v34_phase2_session_receive(&answer, cq[old], PACKET);
        }
        v34_phase2_session_generate(&call, cq[slot], PACKET);
        v34_phase2_session_generate(&answer, aq[slot], PACKET);
    }
    if (!v34_phase2_session_complete(&call) ||
        !v34_phase2_session_complete(&answer))
        fprintf(stderr, "phase2 states call %d/%d/%d tx%d det %d/%d local%d answer %d/%d/%d tx%d\n",
                call.state, call.ranging.state, call.probing.rx_state,
                call.probing.tx_state,
                call.probing.probe_rx.ready, call.probing.probe_rx.signal,
                call.probing.local_info1c_ready,
                answer.state, answer.ranging.state, answer.probing.rx_state,
                answer.probing.tx_state);
    assert(v34_phase2_session_complete(&call));
    assert(v34_phase2_session_complete(&answer));
    assert(v34_phase2_session_round_trip_samples(&call) == 3200u);
    assert(v34_phase2_session_round_trip_samples(&answer) == 3200u);
    assert(v34_phase2_session_info1a(&call) != NULL);
    assert(v34_phase2_session_info1a(&call)->projected_rate_2400 == 14u);
    assert(v34_phase2_session_info1a(&call)->answer_symbol_rate ==
           V34_SYMBOL_3429);
    assert(v34_phase2_session_info1a(&call)->call_symbol_rate ==
           V34_SYMBOL_3429);
    {
        v34_mode call_mode, answer_mode;
        assert(v34_phase2_session_mode(&call, &call_mode, NULL, NULL));
        assert(v34_phase2_session_mode(&answer, &answer_mode, NULL, NULL));
        assert(call_mode.tx_rate == 33600u && call_mode.rx_rate == 33600u);
        assert(answer_mode.tx_rate == 33600u && answer_mode.rx_rate == 33600u);
        assert(call_mode.tx_symbol == V34_SYMBOL_3429);
        assert(answer_mode.rx_symbol == V34_SYMBOL_3429);
    }
    printf("v34 complete Phase 2: RTD 400 ms, 33600/33600 in %u packets\n",
           packet);
    return 0;
}
