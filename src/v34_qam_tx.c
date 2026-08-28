#include "v34_qam_tx.h"

#include "pcma.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool v34_qam_tx_init(v34_qam_tx *tx, v34_symbol_rate rate,
                     bool high_carrier, unsigned sample_rate,
                     double coordinate_scale)
{
    double carrier = v34_carrier_hz(rate, high_carrier);

    if (!tx || carrier <= 0.0 || sample_rate == 0 || coordinate_scale <= 0.0)
        return false;
    tx->carrier_phase = 0.0;
    tx->carrier_step = 2.0 * M_PI * carrier / sample_rate;
    tx->coordinate_scale = coordinate_scale;
    tx->point.re = 0;
    tx->point.im = 0;
    return true;
}

void v34_qam_tx_set_point(v34_qam_tx *tx, v34_point point)
{
    if (tx)
        tx->point = point;
}

int16_t v34_qam_tx_sample(v34_qam_tx *tx)
{
    double value;

    if (!tx)
        return 0;
    value = tx->coordinate_scale *
            (tx->point.re * cos(tx->carrier_phase) -
             tx->point.im * sin(tx->carrier_phase));
    tx->carrier_phase += tx->carrier_step;
    if (tx->carrier_phase >= 2.0 * M_PI)
        tx->carrier_phase = fmod(tx->carrier_phase, 2.0 * M_PI);
    if (value > 32767.0)
        value = 32767.0;
    if (value < -32768.0)
        value = -32768.0;
    return (int16_t)lrint(value);
}

uint8_t v34_qam_tx_pcma(v34_qam_tx *tx)
{
    return pcma_encode(v34_qam_tx_sample(tx));
}
