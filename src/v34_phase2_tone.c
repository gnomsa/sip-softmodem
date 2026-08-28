#include "v34_phase2_tone.h"

#include "pcma.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 8000.0
#define DETECTOR_BLOCK 80u
#define STRONG_ENERGY 1.0e10

static double carrier_for(v34_info_modem_role role)
{
    return role == V34_INFO_ANSWER_MODEM ? 2400.0 : 1200.0;
}

void v34_phase2_tone_tx_init(v34_phase2_tone_tx *tx,
                             v34_info_modem_role role)
{
    if (tx == NULL)
        return;
    memset(tx, 0, sizeof(*tx));
    tx->role = role;
    tx->polarity = 1;
}

void v34_phase2_tone_tx_set_active(v34_phase2_tone_tx *tx, bool active)
{
    if (tx != NULL)
        tx->active = active;
}

void v34_phase2_tone_tx_reverse(v34_phase2_tone_tx *tx)
{
    if (tx != NULL)
        tx->polarity = -tx->polarity;
}

void v34_phase2_tone_tx_generate(v34_phase2_tone_tx *tx, uint8_t *pcma,
                                 size_t sample_count)
{
    size_t i;
    if (tx == NULL || pcma == NULL)
        return;
    for (i = 0; i < sample_count; ++i) {
        double sample = 0.0;
        if (tx->active) {
            double level = tx->role == V34_INFO_ANSWER_MODEM ? 8913.0 :
                                                                   10000.0;
            sample = tx->polarity * level * sin(tx->carrier_phase);
            if (tx->role == V34_INFO_ANSWER_MODEM)
                sample += 4467.0 * sin(tx->guard_phase);
        }
        tx->carrier_phase += 2.0 * M_PI * carrier_for(tx->role) / SAMPLE_RATE;
        if (tx->carrier_phase >= 2.0 * M_PI)
            tx->carrier_phase -= 2.0 * M_PI;
        tx->guard_phase += 2.0 * M_PI * 1800.0 / SAMPLE_RATE;
        if (tx->guard_phase >= 2.0 * M_PI)
            tx->guard_phase -= 2.0 * M_PI;
        pcma[i] = pcma_encode((int16_t)sample);
    }
}

void v34_phase2_tone_rx_init(v34_phase2_tone_rx *rx,
                             v34_info_modem_role remote_role)
{
    if (rx == NULL)
        return;
    memset(rx, 0, sizeof(*rx));
    rx->remote_role = remote_role;
}

static void finish_block(v34_phase2_tone_rx *rx)
{
    double energy = rx->i_acc * rx->i_acc + rx->q_acc * rx->q_acc;
    bool strong = energy >= STRONG_ENERGY;

    if (strong) {
        if (rx->previous_strong) {
            double dot = rx->previous_i * rx->i_acc +
                         rx->previous_q * rx->q_acc;
            if (dot < 0.0)
                ++rx->reversals;
        }
        rx->previous_i = rx->i_acc;
        rx->previous_q = rx->q_acc;
        rx->previous_strong = true;
        ++rx->strong_blocks;
        rx->weak_blocks = 0u;
        if (rx->strong_blocks >= 3u)
            rx->present = true;
    } else {
        rx->strong_blocks = 0u;
        ++rx->weak_blocks;
        rx->previous_strong = false;
        if (rx->weak_blocks >= 3u)
            rx->present = false;
    }
    rx->i_acc = 0.0;
    rx->q_acc = 0.0;
    rx->block_samples = 0u;
}

void v34_phase2_tone_rx_process(v34_phase2_tone_rx *rx, const uint8_t *pcma,
                                size_t sample_count)
{
    size_t i;
    double carrier;
    if (rx == NULL || pcma == NULL)
        return;
    carrier = carrier_for(rx->remote_role);
    for (i = 0; i < sample_count; ++i) {
        double sample = pcma_decode(pcma[i]);
        rx->i_acc += sample * cos(rx->carrier_phase);
        rx->q_acc += sample * sin(rx->carrier_phase);
        rx->carrier_phase += 2.0 * M_PI * carrier / SAMPLE_RATE;
        if (rx->carrier_phase >= 2.0 * M_PI)
            rx->carrier_phase -= 2.0 * M_PI;
        if (++rx->block_samples == DETECTOR_BLOCK)
            finish_block(rx);
    }
}

bool v34_phase2_tone_rx_present(const v34_phase2_tone_rx *rx)
{
    return rx != NULL && rx->present;
}

unsigned v34_phase2_tone_rx_reversals(const v34_phase2_tone_rx *rx)
{
    return rx == NULL ? 0u : rx->reversals;
}
