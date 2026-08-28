#ifndef SOFTMODEM_V34_TRAINING_RX_H
#define SOFTMODEM_V34_TRAINING_RX_H

#include "v34_caps.h"
#include "v34_timing.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    v34_symbol_clock clock;
    double carrier_phase;
    double carrier_step;
    double yc;
    double ys;
    double cc;
    double ss;
    double cs;
} v34_training_rx;

bool v34_training_rx_init(v34_training_rx *rx, v34_symbol_rate rate,
                          bool high_carrier, unsigned sample_rate);
bool v34_training_rx_pcma(v34_training_rx *rx, uint8_t sample,
                          uint8_t *phase_pi_6, double *magnitude);
bool v34_training_rx_pcma_iq(v34_training_rx *rx, uint8_t sample,
                             double *in_phase, double *quadrature);
void v34_training_rx_track_carrier(v34_training_rx *rx,
                                   double in_phase, double quadrature,
                                   double decided_in_phase,
                                   double decided_quadrature);

#endif
