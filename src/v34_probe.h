#ifndef SOFTMODEM_V34_PROBE_H
#define SOFTMODEM_V34_PROBE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V34_PROBE_TONES 21u
#define V34_PROBE_L1_SAMPLES 1280u

typedef enum {
    V34_PROBE_SILENCE = 0,
    V34_PROBE_L1,
    V34_PROBE_L2
} v34_probe_signal;

typedef struct {
    uint64_t sample_at;
    v34_probe_signal signal;
} v34_probe_tx;

typedef struct {
    double i[V34_PROBE_TONES];
    double q[V34_PROBE_TONES];
    double amplitude[V34_PROBE_TONES];
    uint64_t sample_at;
    size_t target_samples;
    size_t samples;
    bool ready;
} v34_probe_rx;

extern const unsigned v34_probe_frequency[V34_PROBE_TONES];
extern const unsigned v34_probe_phase_degrees[V34_PROBE_TONES];

void v34_probe_tx_init(v34_probe_tx *tx);
void v34_probe_tx_set_signal(v34_probe_tx *tx, v34_probe_signal signal);
void v34_probe_tx_generate(v34_probe_tx *tx, uint8_t *pcma,
                           size_t sample_count);

bool v34_probe_rx_init(v34_probe_rx *rx, size_t sample_count);
void v34_probe_rx_process(v34_probe_rx *rx, const uint8_t *pcma,
                          size_t sample_count);
bool v34_probe_rx_ready(const v34_probe_rx *rx);
double v34_probe_rx_amplitude(const v34_probe_rx *rx, unsigned tone);

#endif
