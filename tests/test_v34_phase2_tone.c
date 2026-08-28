#include "v34_phase2_tone.h"

#include <assert.h>
#include <stdio.h>

static void exercise(v34_info_modem_role role)
{
    v34_phase2_tone_tx tx;
    v34_phase2_tone_rx rx;
    uint8_t packet[160];
    unsigned i;

    v34_phase2_tone_tx_init(&tx, role);
    v34_phase2_tone_rx_init(&rx, role);

    v34_phase2_tone_tx_generate(&tx, packet, sizeof(packet));
    v34_phase2_tone_rx_process(&rx, packet, sizeof(packet));
    assert(!v34_phase2_tone_rx_present(&rx));

    v34_phase2_tone_tx_set_active(&tx, true);
    for (i = 0; i < 4u; ++i) {
        v34_phase2_tone_tx_generate(&tx, packet, sizeof(packet));
        /* Exercise retention across non-packet receiver calls. */
        v34_phase2_tone_rx_process(&rx, packet, 73u);
        v34_phase2_tone_rx_process(&rx, packet + 73u,
                                    sizeof(packet) - 73u);
    }
    assert(v34_phase2_tone_rx_present(&rx));
    assert(v34_phase2_tone_rx_reversals(&rx) == 0u);

    v34_phase2_tone_tx_reverse(&tx);
    for (i = 0; i < 3u; ++i) {
        v34_phase2_tone_tx_generate(&tx, packet, sizeof(packet));
        v34_phase2_tone_rx_process(&rx, packet, sizeof(packet));
    }
    assert(v34_phase2_tone_rx_present(&rx));
    assert(v34_phase2_tone_rx_reversals(&rx) == 1u);

    v34_phase2_tone_tx_set_active(&tx, false);
    for (i = 0; i < 3u; ++i) {
        v34_phase2_tone_tx_generate(&tx, packet, sizeof(packet));
        v34_phase2_tone_rx_process(&rx, packet, sizeof(packet));
    }
    assert(!v34_phase2_tone_rx_present(&rx));
}

int main(void)
{
    exercise(V34_INFO_CALL_MODEM);
    exercise(V34_INFO_ANSWER_MODEM);
    puts("v34 Phase 2 A/B tone and reversal tests: ok");
    return 0;
}
