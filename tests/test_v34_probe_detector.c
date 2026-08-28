#include "v34_phase2_tone.h"
#include "v34_probe.h"

#include <assert.h>
#include <stdio.h>

static void exercise(v34_probe_signal expected)
{
    v34_phase2_tone_tx tone;
    v34_probe_tx probe;
    v34_probe_detector detector;
    uint8_t packet[160];
    unsigned i;

    v34_phase2_tone_tx_init(&tone, V34_INFO_ANSWER_MODEM);
    v34_phase2_tone_tx_set_active(&tone, true);
    v34_probe_tx_init(&probe);
    v34_probe_detector_init(&detector);

    /* Tone A and its 1800-Hz guard must not look like a line probe. */
    for (i = 0; i < 5u; ++i) {
        v34_phase2_tone_tx_generate(&tone, packet, sizeof(packet));
        v34_probe_detector_process(&detector, packet, 61u);
        v34_probe_detector_process(&detector, packet + 61u,
                                   sizeof(packet) - 61u);
    }
    assert(!v34_probe_detector_ready(&detector));

    v34_probe_tx_set_signal(&probe, expected);
    for (i = 0; i < 12u && !v34_probe_detector_ready(&detector); ++i) {
        v34_probe_tx_generate(&probe, packet, sizeof(packet));
        v34_probe_detector_process(&detector, packet, 73u);
        v34_probe_detector_process(&detector, packet + 73u,
                                   sizeof(packet) - 73u);
    }
    assert(v34_probe_detector_ready(&detector));
    assert(v34_probe_detector_signal(&detector) == expected);
    assert(v34_probe_detector_measurement(&detector) != NULL);
}

int main(void)
{
    exercise(V34_PROBE_L1);
    exercise(V34_PROBE_L2);
    puts("v34 autonomous L1/L2 detector: tone rejection and PCMA pass");
    return 0;
}
