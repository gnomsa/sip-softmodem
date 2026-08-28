#include "v34_info_modem.h"

#include "pcma.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 8000u
#define BIT_RATE 600u
#define CALL_CARRIER_HZ 1200.0
#define ANSWER_CARRIER_HZ 2400.0
#define ANSWER_GUARD_HZ 1800.0
#define CALL_LEVEL 10000.0
/* Answer carrier is nominal -1 dB; its guard tone is nominal -7 dB. */
#define ANSWER_LEVEL 8913.0
#define GUARD_LEVEL 4467.0
#define CARRIER_THRESHOLD 1000

static unsigned frame_bit(const uint8_t *frame, size_t at)
{
    return (frame[at / 8u] >> (at % 8u)) & 1u;
}

static void store_bit(uint8_t *frame, size_t at, unsigned bit)
{
    uint8_t mask = (uint8_t)(1u << (at % 8u));
    if (bit != 0u)
        frame[at / 8u] |= mask;
    else
        frame[at / 8u] &= (uint8_t)~mask;
}

static double carrier_for(v34_info_modem_role role)
{
    return role == V34_INFO_ANSWER_MODEM ? ANSWER_CARRIER_HZ :
                                           CALL_CARRIER_HZ;
}

void v34_info_modem_tx_init(v34_info_modem_tx *tx,
                            v34_info_modem_role role)
{
    if (tx == NULL)
        return;
    memset(tx, 0, sizeof(*tx));
    tx->role = role;
    tx->polarity = 1;
}

bool v34_info_modem_tx_start(v34_info_modem_tx *tx, const uint8_t *frame,
                             size_t bit_count)
{
    size_t byte_count;
    if (tx == NULL || frame == NULL || bit_count == 0u ||
        bit_count > V34_INFO_MODEM_MAX_BITS)
        return false;
    byte_count = (bit_count + 7u) / 8u;
    memset(tx->frame, 0, sizeof(tx->frame));
    memcpy(tx->frame, frame, byte_count);
    tx->bit_count = bit_count;
    tx->bit_at = 0u;
    tx->bit_clock = 0u;
    tx->carrier_phase = 0.0;
    tx->guard_phase = 0.0;
    tx->polarity = 1;
    tx->active = true;
    return true;
}

void v34_info_modem_tx_generate(v34_info_modem_tx *tx, uint8_t *pcma,
                                size_t sample_count)
{
    size_t i;
    if (tx == NULL || pcma == NULL)
        return;
    for (i = 0; i < sample_count; ++i) {
        double sample = 0.0;
        if (tx->active) {
            double carrier = carrier_for(tx->role);
            sample = tx->polarity * sin(tx->carrier_phase) *
                     (tx->role == V34_INFO_ANSWER_MODEM ? ANSWER_LEVEL :
                                                               CALL_LEVEL);
            tx->carrier_phase += 2.0 * M_PI * carrier / SAMPLE_RATE;
            if (tx->carrier_phase >= 2.0 * M_PI)
                tx->carrier_phase -= 2.0 * M_PI;
            if (tx->role == V34_INFO_ANSWER_MODEM) {
                sample += sin(tx->guard_phase) * GUARD_LEVEL;
                tx->guard_phase += 2.0 * M_PI * ANSWER_GUARD_HZ / SAMPLE_RATE;
                if (tx->guard_phase >= 2.0 * M_PI)
                    tx->guard_phase -= 2.0 * M_PI;
            }

            tx->bit_clock += BIT_RATE;
            if (tx->bit_clock >= SAMPLE_RATE) {
                tx->bit_clock -= SAMPLE_RATE;
                if (tx->bit_at == tx->bit_count) {
                    tx->active = false;
                } else {
                    if (frame_bit(tx->frame, tx->bit_at) != 0u)
                        tx->polarity = -tx->polarity;
                    ++tx->bit_at;
                }
            }
        }
        pcma[i] = pcma_encode((int16_t)sample);
    }
}

bool v34_info_modem_tx_done(const v34_info_modem_tx *tx)
{
    return tx != NULL && !tx->active;
}

void v34_info_modem_rx_init(v34_info_modem_rx *rx,
                            v34_info_modem_role remote_role,
                            size_t expected_bits)
{
    if (rx == NULL)
        return;
    memset(rx, 0, sizeof(*rx));
    rx->remote_role = remote_role;
    if (expected_bits <= V34_INFO_MODEM_MAX_BITS)
        rx->expected_bits = expected_bits;
}

static void finish_rx_interval(v34_info_modem_rx *rx)
{
    if (!rx->have_reference) {
        rx->previous_i = rx->i_acc;
        rx->previous_q = rx->q_acc;
        rx->have_reference = true;
    } else if (rx->bit_count < rx->expected_bits) {
        double correlation = rx->previous_i * rx->i_acc +
                             rx->previous_q * rx->q_acc;
        store_bit(rx->frame, rx->bit_count, correlation < 0.0);
        ++rx->bit_count;
        rx->previous_i = rx->i_acc;
        rx->previous_q = rx->q_acc;
        if (rx->bit_count == rx->expected_bits)
            rx->ready = true;
    }
    rx->i_acc = 0.0;
    rx->q_acc = 0.0;
}

void v34_info_modem_rx_process(v34_info_modem_rx *rx, const uint8_t *pcma,
                               size_t sample_count)
{
    size_t i;
    double carrier;
    if (rx == NULL || pcma == NULL || rx->expected_bits == 0u || rx->ready)
        return;
    carrier = carrier_for(rx->remote_role);
    for (i = 0; i < sample_count && !rx->ready; ++i) {
        int16_t decoded = pcma_decode(pcma[i]);
        double sample;
        if (!rx->carrier_seen) {
            if (decoded > -CARRIER_THRESHOLD && decoded < CARRIER_THRESHOLD)
                continue;
            rx->carrier_seen = true;
        }
        sample = decoded;
        rx->i_acc += sample * cos(rx->carrier_phase);
        rx->q_acc += sample * sin(rx->carrier_phase);
        rx->carrier_phase += 2.0 * M_PI * carrier / SAMPLE_RATE;
        if (rx->carrier_phase >= 2.0 * M_PI)
            rx->carrier_phase -= 2.0 * M_PI;
        rx->bit_clock += BIT_RATE;
        if (rx->bit_clock >= SAMPLE_RATE) {
            rx->bit_clock -= SAMPLE_RATE;
            finish_rx_interval(rx);
        }
    }
}

bool v34_info_modem_rx_ready(const v34_info_modem_rx *rx)
{
    return rx != NULL && rx->ready;
}

bool v34_info_modem_rx_read(v34_info_modem_rx *rx, uint8_t *frame,
                            size_t capacity, size_t *bit_count)
{
    size_t byte_count;
    if (rx == NULL || frame == NULL || !rx->ready)
        return false;
    byte_count = (rx->expected_bits + 7u) / 8u;
    if (capacity < byte_count)
        return false;
    memcpy(frame, rx->frame, byte_count);
    if (bit_count != NULL)
        *bit_count = rx->expected_bits;
    return true;
}
