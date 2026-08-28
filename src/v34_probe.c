#include "v34_probe.h"

#include "pcma.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 8000.0
#define NOMINAL_TONE_AMPLITUDE 1200.0
#define L1_GAIN 1.9952623149688795 /* +6 dB */

const unsigned v34_probe_frequency[V34_PROBE_TONES] = {
    150, 300, 450, 600, 750, 1050, 1350,
    1500, 1650, 1950, 2100, 2250, 2550, 2700,
    2850, 3000, 3150, 3300, 3450, 3600, 3750
};

const unsigned v34_probe_phase_degrees[V34_PROBE_TONES] = {
    0, 180, 0, 0, 0, 0, 0,
    0, 180, 0, 0, 180, 0, 180,
    0, 180, 180, 180, 180, 0, 0
};

void v34_probe_tx_init(v34_probe_tx *tx)
{
    if (tx != NULL)
        memset(tx, 0, sizeof(*tx));
}

void v34_probe_tx_set_signal(v34_probe_tx *tx, v34_probe_signal signal)
{
    if (tx == NULL)
        return;
    tx->signal = signal;
    tx->sample_at = 0u;
}

void v34_probe_tx_generate(v34_probe_tx *tx, uint8_t *pcma,
                           size_t sample_count)
{
    size_t n;
    if (tx == NULL || pcma == NULL)
        return;
    for (n = 0; n < sample_count; ++n) {
        double sample = 0.0;
        unsigned tone;
        if (tx->signal != V34_PROBE_SILENCE) {
            double gain = tx->signal == V34_PROBE_L1 ? L1_GAIN : 1.0;
            for (tone = 0; tone < V34_PROBE_TONES; ++tone) {
                double phase = 2.0 * M_PI *
                    (double)v34_probe_frequency[tone] *
                    (double)tx->sample_at / SAMPLE_RATE;
                if (v34_probe_phase_degrees[tone] == 180u)
                    phase += M_PI;
                sample += NOMINAL_TONE_AMPLITUDE * gain * cos(phase);
            }
        }
        if (sample > 32767.0)
            sample = 32767.0;
        else if (sample < -32768.0)
            sample = -32768.0;
        pcma[n] = pcma_encode((int16_t)sample);
        ++tx->sample_at;
    }
}

bool v34_probe_rx_init(v34_probe_rx *rx, size_t sample_count)
{
    if (rx == NULL || sample_count == 0u)
        return false;
    memset(rx, 0, sizeof(*rx));
    rx->target_samples = sample_count;
    return true;
}

static void finish(v34_probe_rx *rx)
{
    unsigned tone;
    for (tone = 0; tone < V34_PROBE_TONES; ++tone)
        rx->amplitude[tone] = 2.0 * hypot(rx->i[tone], rx->q[tone]) /
                              (double)rx->samples;
    rx->ready = true;
}

void v34_probe_rx_process(v34_probe_rx *rx, const uint8_t *pcma,
                          size_t sample_count)
{
    size_t n;
    if (rx == NULL || pcma == NULL || rx->ready)
        return;
    for (n = 0; n < sample_count && !rx->ready; ++n) {
        double sample = pcma_decode(pcma[n]);
        unsigned tone;
        for (tone = 0; tone < V34_PROBE_TONES; ++tone) {
            double phase = 2.0 * M_PI *
                (double)v34_probe_frequency[tone] *
                (double)rx->sample_at / SAMPLE_RATE;
            rx->i[tone] += sample * cos(phase);
            rx->q[tone] += sample * sin(phase);
        }
        ++rx->sample_at;
        if (++rx->samples == rx->target_samples)
            finish(rx);
    }
}

bool v34_probe_rx_ready(const v34_probe_rx *rx)
{
    return rx != NULL && rx->ready;
}

double v34_probe_rx_amplitude(const v34_probe_rx *rx, unsigned tone)
{
    return rx != NULL && rx->ready && tone < V34_PROBE_TONES ?
        rx->amplitude[tone] : 0.0;
}
