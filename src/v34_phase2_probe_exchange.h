#ifndef SOFTMODEM_V34_PHASE2_PROBE_EXCHANGE_H
#define SOFTMODEM_V34_PHASE2_PROBE_EXCHANGE_H

#include "v34_caps.h"
#include "v34_info.h"
#include "v34_info_modem.h"
#include "v34_probe.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    V34_PHASE2_PROBE_TX_WAIT = 0,
    V34_PHASE2_PROBE_TX_L1,
    V34_PHASE2_PROBE_TX_L2,
    V34_PHASE2_PROBE_TX_INFO1C,
    V34_PHASE2_PROBE_TX_INFO1A,
    V34_PHASE2_PROBE_TX_DONE
} v34_phase2_probe_tx_state;

typedef enum {
    V34_PHASE2_PROBE_RX_L1 = 0,
    V34_PHASE2_PROBE_RX_L2,
    V34_PHASE2_PROBE_RX_L2_TAIL,
    V34_PHASE2_PROBE_RX_INFO1C,
    V34_PHASE2_PROBE_RX_INFO1A,
    V34_PHASE2_PROBE_RX_DONE,
    V34_PHASE2_PROBE_RX_FAILED
} v34_phase2_probe_rx_state;

typedef struct {
    v34_info_modem_role role;
    uint8_t allowed_symbols;
    uint16_t allowed_rates;
    unsigned maximum_rate;
    unsigned maximum_symbol_difference;
    unsigned round_trip_samples;
    v34_phase2_probe_tx_state tx_state;
    v34_phase2_probe_rx_state rx_state;
    unsigned tx_stage_samples;
    unsigned rx_tail_samples;
    v34_probe_tx probe_tx;
    v34_probe_detector probe_rx;
    v34_probe_rx remote_probe;
    v34_info_modem_tx info_tx;
    v34_info_modem_rx info_rx;
    v34_info1c local_info1c;
    v34_info1c peer_info1c;
    v34_info1a negotiated_info1a;
    bool local_info1c_ready;
    bool peer_info1c_ready;
    bool info1a_ready;
} v34_phase2_probe_exchange;

bool v34_phase2_probe_exchange_init(v34_phase2_probe_exchange *exchange,
                                    v34_info_modem_role role,
                                    uint8_t allowed_symbols,
                                    uint16_t allowed_rates,
                                    unsigned maximum_rate,
                                    unsigned maximum_symbol_difference,
                                    unsigned round_trip_samples);
void v34_phase2_probe_exchange_generate(v34_phase2_probe_exchange *exchange,
                                        uint8_t *pcma,
                                        size_t sample_count);
void v34_phase2_probe_exchange_receive(v34_phase2_probe_exchange *exchange,
                                       const uint8_t *pcma,
                                       size_t sample_count);
bool v34_phase2_probe_exchange_complete(
    const v34_phase2_probe_exchange *exchange);
const v34_info1a *v34_phase2_probe_exchange_info1a(
    const v34_phase2_probe_exchange *exchange);

#endif
