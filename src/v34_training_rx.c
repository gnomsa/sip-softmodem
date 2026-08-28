#include "v34_training_rx.h"

#include "pcma.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool v34_training_rx_init(v34_training_rx *rx, v34_symbol_rate rate,
                          bool high_carrier, unsigned sample_rate)
{
    double carrier = v34_carrier_hz(rate, high_carrier);
    if (rx == NULL || carrier <= 0.0 || sample_rate == 0)
        return false;
    memset(rx, 0, sizeof(*rx));
    rx->carrier_step = 2.0 * M_PI * carrier / sample_rate;
    return v34_symbol_clock_init(&rx->clock, rate, sample_rate);
}

bool v34_training_rx_pcma_iq(v34_training_rx *rx, uint8_t sample,
                             double *in_phase, double *quadrature)
{
    double x, c, s;
    if (rx == NULL || in_phase == NULL || quadrature == NULL)
        return false;
    x = pcma_decode(sample);
    c = cos(rx->carrier_phase);
    s = sin(rx->carrier_phase);
    rx->yc += x * c;
    rx->ys -= x * s;
    rx->cc += c * c;
    rx->ss += s * s;
    rx->cs += c * s;
    rx->carrier_phase += rx->carrier_step;
    if (rx->carrier_phase >= 2.0 * M_PI)
        rx->carrier_phase = fmod(rx->carrier_phase, 2.0 * M_PI);
    if (!v34_symbol_clock_tick(&rx->clock))
        return false;
    {
        double det = rx->cc * rx->ss - rx->cs * rx->cs;
        *in_phase = 0.0;
        *quadrature = 0.0;
        if (fabs(det) > 1e-9) {
            *in_phase = (rx->ss * rx->yc + rx->cs * rx->ys) / det;
            *quadrature = (rx->cs * rx->yc + rx->cc * rx->ys) / det;
        }
    }
    rx->yc = rx->ys = rx->cc = rx->ss = rx->cs = 0.0;
    return true;
}

bool v34_training_rx_pcma(v34_training_rx *rx, uint8_t sample,
                          uint8_t *phase_pi_6, double *magnitude)
{
    double i;
    double q;
    double angle;
    long decision;

    if (!phase_pi_6 || !v34_training_rx_pcma_iq(rx, sample, &i, &q))
        return false;
    angle = atan2(q, i);
    if (angle < 0.0)
        angle += 2.0 * M_PI;
    decision = lround(angle * 6.0 / M_PI) % 12;
    *phase_pi_6 = (uint8_t)decision;
    if (magnitude)
        *magnitude = hypot(i, q);
    return true;
}
