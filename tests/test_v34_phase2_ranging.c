#include "v34_phase2_ranging.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PACKET 160u
#define DELAY_PACKETS 10u

int main(void)
{
    const v34_info0 call_info = {
        true, true, true, true, false, true, true, true, false, 3,
        true, true, V34_CLOCK_INTERNAL, false
    };
    const v34_info0 answer_info = {
        true, false, true, false, true, true, false, true, true, 2,
        true, false, V34_CLOCK_RECEIVE, true
    };
    v34_phase2_ranging call, answer;
    uint8_t call_queue[DELAY_PACKETS][PACKET];
    uint8_t answer_queue[DELAY_PACKETS][PACKET];
    unsigned packet;

    assert(v34_phase2_ranging_init(&call, V34_INFO_CALL_MODEM, &call_info));
    assert(v34_phase2_ranging_init(&answer, V34_INFO_ANSWER_MODEM,
                                   &answer_info));

    for (packet = 0; packet < 300u &&
         (!v34_phase2_ranging_complete(&call) ||
          !v34_phase2_ranging_complete(&answer)); ++packet) {
        unsigned slot = packet % DELAY_PACKETS;
        if (packet >= DELAY_PACKETS) {
            unsigned receive_slot = (packet - DELAY_PACKETS) % DELAY_PACKETS;
            v34_phase2_ranging_receive(&call, answer_queue[receive_slot],
                                       PACKET);
            v34_phase2_ranging_receive(&answer, call_queue[receive_slot],
                                       PACKET);
        }
        v34_phase2_ranging_generate(&call, call_queue[slot], PACKET);
        v34_phase2_ranging_generate(&answer, answer_queue[slot], PACKET);
    }

    if (!v34_phase2_ranging_complete(&call) ||
        !v34_phase2_ranging_complete(&answer))
        fprintf(stderr, "states %d/%d rev %u/%u peer %d/%d packet %u\n",
                call.state, answer.state, call.observed_reversals,
                answer.observed_reversals, call.peer_info_ready,
                answer.peer_info_ready, packet);
    assert(v34_phase2_ranging_complete(&call));
    assert(v34_phase2_ranging_complete(&answer));
    assert(v34_phase2_ranging_peer_info(&call) != NULL);
    assert(v34_phase2_ranging_peer_info(&answer) != NULL);
    assert(v34_phase2_ranging_peer_info(&call)->acknowledge);
    assert(v34_phase2_ranging_peer_info(&answer)->symbol_2800);
    assert(v34_phase2_ranging_round_trip_samples(&call) >= 2880u);
    assert(v34_phase2_ranging_round_trip_samples(&call) <= 3520u);
    assert(v34_phase2_ranging_round_trip_samples(&answer) >= 2880u);
    assert(v34_phase2_ranging_round_trip_samples(&answer) <= 3520u);

    printf("v34 Phase 2 ranging: INFO0 and A/B reversals, RTD %u/%u samples\n",
           v34_phase2_ranging_round_trip_samples(&call),
           v34_phase2_ranging_round_trip_samples(&answer));
    return 0;
}
