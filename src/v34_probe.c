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

void v34_probe_detector_init(v34_probe_detector *detector)
{
    if (detector != NULL)
        memset(detector, 0, sizeof(*detector));
}

static double block_amplitude(const uint8_t *pcma, unsigned frequency)
{
    double i = 0.0, q = 0.0;
    size_t n;
    for (n = 0; n < V34_PROBE_DETECT_BLOCK; ++n) {
        double sample = pcma_decode(pcma[n]);
        double phase = 2.0 * M_PI * frequency * n / SAMPLE_RATE;
        i += sample * cos(phase);
        q += sample * sin(phase);
    }
    return 2.0 * hypot(i, q) / V34_PROBE_DETECT_BLOCK;
}

static bool probe_block(const uint8_t *pcma)
{
    static const unsigned omitted[] = {900u, 1200u, 1800u, 2400u};
    double probe_energy = 0.0, omitted_energy = 0.0;
    unsigned i;
    for (i = 0; i < V34_PROBE_TONES; ++i) {
        double amplitude = block_amplitude(pcma, v34_probe_frequency[i]);
        probe_energy += amplitude * amplitude;
    }
    for (i = 0; i < sizeof(omitted) / sizeof(omitted[0]); ++i) {
        double amplitude = block_amplitude(pcma, omitted[i]);
        omitted_energy += amplitude * amplitude;
    }
    return probe_energy > 21.0 * 300.0 * 300.0 &&
           probe_energy > omitted_energy * 4.0;
}

static void classify_probe(v34_probe_detector *detector)
{
    double average = 0.0;
    unsigned tone;
    for (tone = 0; tone < V34_PROBE_TONES; ++tone)
        average += v34_probe_rx_amplitude(&detector->measurement, tone);
    average /= V34_PROBE_TONES;
    detector->signal = average >= 1700.0 ? V34_PROBE_L1 : V34_PROBE_L2;
    detector->ready = true;
}

void v34_probe_detector_process(v34_probe_detector *detector,
                                const uint8_t *pcma, size_t sample_count)
{
    size_t at = 0u;
    if (detector == NULL || pcma == NULL || detector->ready)
        return;
    while (at < sample_count && !detector->ready) {
        if (detector->measuring) {
            size_t before = detector->measurement.samples;
            v34_probe_rx_process(&detector->measurement, pcma + at,
                                 sample_count - at);
            at += detector->measurement.samples - before;
            if (v34_probe_rx_ready(&detector->measurement))
                classify_probe(detector);
        } else {
            size_t room = V34_PROBE_DETECT_BLOCK - detector->scan_samples;
            size_t take = sample_count - at < room ? sample_count - at : room;
            memcpy(detector->scan + detector->scan_samples, pcma + at, take);
            detector->scan_samples += take;
            at += take;
            if (detector->scan_samples == V34_PROBE_DETECT_BLOCK) {
                if (probe_block(detector->scan)) {
                    (void)v34_probe_rx_init(&detector->measurement,
                                            V34_PROBE_L1_SAMPLES);
                    v34_probe_rx_process(&detector->measurement,
                                         detector->scan,
                                         V34_PROBE_DETECT_BLOCK);
                    detector->measuring = true;
                }
                detector->scan_samples = 0u;
            }
        }
    }
}

bool v34_probe_detector_ready(const v34_probe_detector *detector)
{
    return detector != NULL && detector->ready;
}

v34_probe_signal v34_probe_detector_signal(
    const v34_probe_detector *detector)
{
    return detector != NULL && detector->ready ? detector->signal :
                                                  V34_PROBE_SILENCE;
}

const v34_probe_rx *v34_probe_detector_measurement(
    const v34_probe_detector *detector)
{
    return detector != NULL && detector->ready ? &detector->measurement : NULL;
}
