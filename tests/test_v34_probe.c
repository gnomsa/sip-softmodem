#include "pcma.h"
#include "v34_probe.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static double measure(const uint8_t *pcma, size_t count, unsigned frequency)
{
    double i = 0.0, q = 0.0;
    size_t n;
    for (n = 0; n < count; ++n) {
        double x = pcma_decode(pcma[n]);
        double phase = 2.0 * 3.14159265358979323846 * frequency * n / 8000.0;
        i += x * cos(phase);
        q += x * sin(phase);
    }
    return 2.0 * hypot(i, q) / count;
}

static double measure_real(const uint8_t *pcma, size_t count,
                           unsigned frequency)
{
    double i = 0.0;
    size_t n;
    for (n = 0; n < count; ++n) {
        double phase = 2.0 * 3.14159265358979323846 * frequency * n / 8000.0;
        i += pcma_decode(pcma[n]) * cos(phase);
    }
    return 2.0 * i / count;
}

static double run(v34_probe_signal signal)
{
    v34_probe_tx tx;
    v34_probe_rx rx;
    uint8_t pcma[V34_PROBE_L1_SAMPLES];
    double average = 0.0;
    unsigned tone;

    v34_probe_tx_init(&tx);
    v34_probe_tx_set_signal(&tx, signal);
    v34_probe_tx_generate(&tx, pcma, 317u);
    v34_probe_tx_generate(&tx, pcma + 317u,
                          V34_PROBE_L1_SAMPLES - 317u);
    assert(v34_probe_rx_init(&rx, V34_PROBE_L1_SAMPLES));
    v34_probe_rx_process(&rx, pcma, 151u);
    v34_probe_rx_process(&rx, pcma + 151u,
                         V34_PROBE_L1_SAMPLES - 151u);
    assert(v34_probe_rx_ready(&rx));

    for (tone = 0; tone < V34_PROBE_TONES; ++tone) {
        double amplitude = v34_probe_rx_amplitude(&rx, tone);
        double real = measure_real(pcma, sizeof(pcma),
                                   v34_probe_frequency[tone]);
        assert(amplitude > 700.0);
        assert(v34_probe_phase_degrees[tone] == 180u ? real < -700.0 :
                                                        real > 700.0);
        average += amplitude;
    }
    average /= V34_PROBE_TONES;

    /* The four explicitly omitted 150-Hz grid tones remain suppressed. */
    assert(measure(pcma, sizeof(pcma), 900u) < average * 0.20);
    assert(measure(pcma, sizeof(pcma), 1200u) < average * 0.20);
    assert(measure(pcma, sizeof(pcma), 1800u) < average * 0.20);
    assert(measure(pcma, sizeof(pcma), 2400u) < average * 0.20);
    return average;
}

int main(void)
{
    double l1 = run(V34_PROBE_L1);
    double l2 = run(V34_PROBE_L2);
    double ratio_db = 20.0 * log10(l1 / l2);

    assert(ratio_db > 5.5 && ratio_db < 6.5);
    printf("v34 L1/L2 probe: 21 tones, L1-L2 %.2f dB\n", ratio_db);
    return 0;
}
