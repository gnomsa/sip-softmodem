#ifndef SOFTMODEM_V34_TRAINING_TX_H
#define SOFTMODEM_V34_TRAINING_TX_H
#include "v34_caps.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct { double carrier_phase, carrier_step, amplitude; uint8_t symbol_phase; } v34_training_tx;
bool v34_training_tx_init(v34_training_tx *tx, v34_symbol_rate rate,
                          bool high_carrier, unsigned sample_rate, double amplitude);
void v34_training_tx_set_phase(v34_training_tx *tx, uint8_t phase_pi_6);
int16_t v34_training_tx_sample(v34_training_tx *tx);
uint8_t v34_training_tx_pcma(v34_training_tx *tx);
#endif
