#include "v34_phase2_probe_exchange.h"

#include "pcma.h"
#include "v34_phase2.h"

#include <string.h>

#define L2_SAMPLES 4400u

bool v34_phase2_probe_exchange_init(v34_phase2_probe_exchange *exchange,
                                    v34_info_modem_role role,
                                    uint8_t allowed_symbols,
                                    uint16_t allowed_rates,
                                    unsigned maximum_rate,
                                    unsigned maximum_symbol_difference,
                                    unsigned round_trip_samples)
{
    if (exchange == NULL || maximum_symbol_difference > 5u ||
        (allowed_symbols & V34_SYMBOL_ALL_MASK) == 0u ||
        (allowed_rates & V34_RATE_ALL_MASK) == 0u)
        return false;
    memset(exchange, 0, sizeof(*exchange));
    exchange->role = role;
    exchange->allowed_symbols = allowed_symbols & V34_SYMBOL_ALL_MASK;
    exchange->allowed_rates = allowed_rates & V34_RATE_ALL_MASK;
    exchange->maximum_rate = maximum_rate;
    exchange->maximum_symbol_difference = maximum_symbol_difference;
    exchange->round_trip_samples = round_trip_samples;
    exchange->tx_state = role == V34_INFO_ANSWER_MODEM ?
                         V34_PHASE2_PROBE_TX_L1 : V34_PHASE2_PROBE_TX_WAIT;
    exchange->rx_state = V34_PHASE2_PROBE_RX_L1;
    v34_probe_tx_init(&exchange->probe_tx);
    v34_probe_detector_init(&exchange->probe_rx);
    if (exchange->tx_state == V34_PHASE2_PROBE_TX_L1)
        v34_probe_tx_set_signal(&exchange->probe_tx, V34_PROBE_L1);
    return true;
}

static bool start_info(v34_phase2_probe_exchange *exchange,
                       v34_phase2_probe_tx_state state)
{
    uint8_t frame[V34_INFO1C_BYTES];
    size_t bits;
    bool encoded;
    v34_info_modem_tx_init(&exchange->info_tx, exchange->role);
    if (state == V34_PHASE2_PROBE_TX_INFO1C) {
        bits = V34_INFO1C_BITS;
        encoded = v34_info1c_encode(&exchange->local_info1c, frame);
    } else {
        bits = V34_INFO1A_BITS;
        encoded = v34_info1a_encode(&exchange->negotiated_info1a, frame);
    }
    if (!encoded || !v34_info_modem_tx_start(&exchange->info_tx, frame, bits))
        return false;
    exchange->tx_state = state;
    return true;
}

static void generate_one(v34_phase2_probe_exchange *exchange, uint8_t *sample)
{
    switch (exchange->tx_state) {
    case V34_PHASE2_PROBE_TX_L1:
        v34_probe_tx_generate(&exchange->probe_tx, sample, 1u);
        if (++exchange->tx_stage_samples == V34_PROBE_L1_SAMPLES) {
            exchange->tx_stage_samples = 0u;
            exchange->tx_state = V34_PHASE2_PROBE_TX_L2;
            v34_probe_tx_set_signal(&exchange->probe_tx, V34_PROBE_L2);
        }
        break;
    case V34_PHASE2_PROBE_TX_L2:
        v34_probe_tx_generate(&exchange->probe_tx, sample, 1u);
        if (++exchange->tx_stage_samples == L2_SAMPLES) {
            exchange->tx_stage_samples = 0u;
            if (exchange->role == V34_INFO_CALL_MODEM) {
                if (!start_info(exchange, V34_PHASE2_PROBE_TX_INFO1C))
                    exchange->rx_state = V34_PHASE2_PROBE_RX_FAILED;
            } else {
                exchange->tx_state = V34_PHASE2_PROBE_TX_WAIT;
            }
        }
        break;
    case V34_PHASE2_PROBE_TX_INFO1C:
    case V34_PHASE2_PROBE_TX_INFO1A:
        v34_info_modem_tx_generate(&exchange->info_tx, sample, 1u);
        if (v34_info_modem_tx_done(&exchange->info_tx))
            exchange->tx_state = V34_PHASE2_PROBE_TX_DONE;
        break;
    default:
        *sample = pcma_encode(0);
        break;
    }
}

void v34_phase2_probe_exchange_generate(v34_phase2_probe_exchange *exchange,
                                        uint8_t *pcma,
                                        size_t sample_count)
{
    size_t i;
    if (exchange == NULL || pcma == NULL)
        return;
    for (i = 0; i < sample_count; ++i)
        generate_one(exchange, &pcma[i]);
}

static void start_local_probe(v34_phase2_probe_exchange *exchange)
{
    exchange->tx_state = V34_PHASE2_PROBE_TX_L1;
    exchange->tx_stage_samples = 0u;
    v34_probe_tx_set_signal(&exchange->probe_tx, V34_PROBE_L1);
}

static void finish_remote_l2(v34_phase2_probe_exchange *exchange)
{
    if (!v34_phase2_make_info1c(&exchange->remote_probe,
                                exchange->allowed_symbols,
                                exchange->allowed_rates,
                                exchange->maximum_rate,
                                &exchange->local_info1c)) {
        exchange->rx_state = V34_PHASE2_PROBE_RX_FAILED;
        return;
    }
    exchange->local_info1c_ready = true;
    if (exchange->role == V34_INFO_CALL_MODEM) {
        start_local_probe(exchange);
        exchange->rx_state = V34_PHASE2_PROBE_RX_INFO1A;
        v34_info_modem_rx_init(&exchange->info_rx,
                               V34_INFO_ANSWER_MODEM, V34_INFO1A_BITS);
    } else {
        exchange->rx_state = V34_PHASE2_PROBE_RX_INFO1C;
        v34_info_modem_rx_init(&exchange->info_rx,
                               V34_INFO_CALL_MODEM, V34_INFO1C_BITS);
    }
}

static void accept_info1c(v34_phase2_probe_exchange *exchange)
{
    uint8_t frame[V34_INFO1C_BYTES];
    v34_phase2_duplex duplex;
    size_t bits;
    if (!v34_info_modem_rx_read(&exchange->info_rx, frame, sizeof(frame),
                                &bits) || bits != V34_INFO1C_BITS ||
        !v34_info1c_decode(frame, &exchange->peer_info1c) ||
        !v34_phase2_select_duplex(&exchange->peer_info1c,
                                  &exchange->local_info1c,
                                  exchange->allowed_symbols,
                                  exchange->allowed_rates,
                                  exchange->maximum_rate,
                                  exchange->maximum_symbol_difference,
                                  &duplex) ||
        !v34_phase2_make_info1a(&duplex, 0u, 0u, 0u, 0,
                                &exchange->negotiated_info1a) ||
        !start_info(exchange, V34_PHASE2_PROBE_TX_INFO1A)) {
        exchange->rx_state = V34_PHASE2_PROBE_RX_FAILED;
        return;
    }
    exchange->peer_info1c_ready = true;
    exchange->info1a_ready = true;
    exchange->rx_state = V34_PHASE2_PROBE_RX_DONE;
}

static void accept_info1a(v34_phase2_probe_exchange *exchange)
{
    uint8_t frame[V34_INFO1A_BYTES];
    size_t bits;
    if (!v34_info_modem_rx_read(&exchange->info_rx, frame, sizeof(frame),
                                &bits) || bits != V34_INFO1A_BITS ||
        !v34_info1a_decode(frame, &exchange->negotiated_info1a)) {
        exchange->rx_state = V34_PHASE2_PROBE_RX_FAILED;
        return;
    }
    exchange->info1a_ready = true;
    exchange->rx_state = V34_PHASE2_PROBE_RX_DONE;
}

static void receive_one(v34_phase2_probe_exchange *exchange, uint8_t sample)
{
    switch (exchange->rx_state) {
    case V34_PHASE2_PROBE_RX_L1:
    case V34_PHASE2_PROBE_RX_L2:
        v34_probe_detector_process(&exchange->probe_rx, &sample, 1u);
        if (v34_probe_detector_ready(&exchange->probe_rx)) {
            v34_probe_signal signal =
                v34_probe_detector_signal(&exchange->probe_rx);
            if (exchange->rx_state == V34_PHASE2_PROBE_RX_L1 &&
                signal == V34_PROBE_L1) {
                exchange->rx_state = V34_PHASE2_PROBE_RX_L2;
                v34_probe_detector_init(&exchange->probe_rx);
            } else if (exchange->rx_state == V34_PHASE2_PROBE_RX_L2 &&
                       signal == V34_PROBE_L2) {
                exchange->remote_probe =
                    *v34_probe_detector_measurement(&exchange->probe_rx);
                exchange->rx_tail_samples =
                    L2_SAMPLES - V34_PROBE_L1_SAMPLES;
                exchange->rx_state = V34_PHASE2_PROBE_RX_L2_TAIL;
            } else {
                exchange->rx_state = V34_PHASE2_PROBE_RX_FAILED;
            }
        }
        break;
    case V34_PHASE2_PROBE_RX_L2_TAIL:
        if (--exchange->rx_tail_samples == 0u)
            finish_remote_l2(exchange);
        break;
    case V34_PHASE2_PROBE_RX_INFO1C:
    case V34_PHASE2_PROBE_RX_INFO1A:
        v34_info_modem_rx_process(&exchange->info_rx, &sample, 1u);
        if (v34_info_modem_rx_ready(&exchange->info_rx)) {
            if (exchange->rx_state == V34_PHASE2_PROBE_RX_INFO1C)
                accept_info1c(exchange);
            else
                accept_info1a(exchange);
        }
        break;
    default:
        break;
    }
}

void v34_phase2_probe_exchange_receive(v34_phase2_probe_exchange *exchange,
                                       const uint8_t *pcma,
                                       size_t sample_count)
{
    size_t i;
    if (exchange == NULL || pcma == NULL ||
        exchange->rx_state == V34_PHASE2_PROBE_RX_FAILED)
        return;
    for (i = 0; i < sample_count; ++i)
        receive_one(exchange, pcma[i]);
}

bool v34_phase2_probe_exchange_complete(
    const v34_phase2_probe_exchange *exchange)
{
    return exchange != NULL && exchange->rx_state == V34_PHASE2_PROBE_RX_DONE &&
           exchange->tx_state == V34_PHASE2_PROBE_TX_DONE;
}

const v34_info1a *v34_phase2_probe_exchange_info1a(
    const v34_phase2_probe_exchange *exchange)
{
    return exchange != NULL && exchange->info1a_ready ?
        &exchange->negotiated_info1a : NULL;
}
