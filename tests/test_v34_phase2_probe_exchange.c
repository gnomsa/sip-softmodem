#include "v34_phase2_probe_exchange.h"

#include <assert.h>
#include <stdio.h>

#define PACKET 160u
#define DELAY 10u

int main(void)
{
    v34_phase2_probe_exchange call, answer;
    uint8_t cq[DELAY][PACKET], aq[DELAY][PACKET];
    unsigned packet;

    assert(v34_phase2_probe_exchange_init(&call, V34_INFO_CALL_MODEM,
        V34_SYMBOL_ALL_MASK, V34_RATE_ALL_MASK, 33600u, 1u, 3200u));
    assert(v34_phase2_probe_exchange_init(&answer, V34_INFO_ANSWER_MODEM,
        V34_SYMBOL_ALL_MASK, V34_RATE_ALL_MASK, 33600u, 1u, 3200u));

    for (packet = 0; packet < 400u &&
         (!v34_phase2_probe_exchange_complete(&call) ||
          !v34_phase2_probe_exchange_complete(&answer)); ++packet) {
        unsigned slot = packet % DELAY;
        if (packet >= DELAY) {
            unsigned old = (packet - DELAY) % DELAY;
            v34_phase2_probe_exchange_receive(&call, aq[old], PACKET);
            v34_phase2_probe_exchange_receive(&answer, cq[old], PACKET);
        }
        v34_phase2_probe_exchange_generate(&call, cq[slot], PACKET);
        v34_phase2_probe_exchange_generate(&answer, aq[slot], PACKET);
    }
    if (!v34_phase2_probe_exchange_complete(&call) ||
        !v34_phase2_probe_exchange_complete(&answer))
        fprintf(stderr, "probe states tx/rx call %d/%d answer %d/%d\n",
                call.tx_state, call.rx_state, answer.tx_state, answer.rx_state);
    assert(v34_phase2_probe_exchange_complete(&call));
    assert(v34_phase2_probe_exchange_complete(&answer));
    assert(v34_phase2_probe_exchange_info1a(&call) != NULL);
    assert(v34_phase2_probe_exchange_info1a(&answer) != NULL);
    assert(v34_phase2_probe_exchange_info1a(&call)->projected_rate_2400 == 14u);
    assert(v34_phase2_probe_exchange_info1a(&call)->answer_symbol_rate ==
           V34_SYMBOL_3429);
    assert(v34_phase2_probe_exchange_info1a(&call)->call_symbol_rate ==
           V34_SYMBOL_3429);
    printf("v34 Phase 2 probes/INFO1: 33600/33600 in %u packets\n", packet);
    return 0;
}
