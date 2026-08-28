#include "v34_training_tx.h"
#include "pcma.h"
#include <math.h>
#include <stddef.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
bool v34_training_tx_init(v34_training_tx *tx, v34_symbol_rate rate,
                          bool high_carrier, unsigned sample_rate, double amplitude)
{
    double carrier = v34_carrier_hz(rate, high_carrier);
    if (tx == NULL || carrier <= 0.0 || sample_rate == 0 || amplitude <= 0.0 || amplitude > 32767.0)
        return false;
    tx->carrier_phase = 0.0;
    tx->carrier_step = 2.0 * M_PI * carrier / sample_rate;
    tx->amplitude = amplitude;
    tx->symbol_phase = 0;
    return true;
}
void v34_training_tx_set_phase(v34_training_tx *tx, uint8_t phase_pi_6)
{
    if (tx != NULL) tx->symbol_phase = (uint8_t)(phase_pi_6 % 12u);
}
int16_t v34_training_tx_sample(v34_training_tx *tx)
{
    double value, phase;
    if (tx == NULL) return 0;
    phase = tx->carrier_phase + M_PI * tx->symbol_phase / 6.0;
    value = tx->amplitude * cos(phase);
    tx->carrier_phase += tx->carrier_step;
    if (tx->carrier_phase >= 2.0 * M_PI) tx->carrier_phase = fmod(tx->carrier_phase, 2.0 * M_PI);
    if (value > 32767.0) value = 32767.0;
    if (value < -32768.0) value = -32768.0;
    return (int16_t)lrint(value);
}
uint8_t v34_training_tx_pcma(v34_training_tx *tx) { return pcma_encode(v34_training_tx_sample(tx)); }
