#ifndef SOFTMODEM_V34_SESSION_H
#define SOFTMODEM_V34_SESSION_H

#include "v34_b1_data_link.h"
#include "v34_phase3_receiver.h"
#include "v34_phase3_stream.h"

typedef enum {
    V34_SESSION_PHASE3,
    V34_SESSION_PHASE4,
    V34_SESSION_B1_DATA,
    V34_SESSION_FAILED
} v34_session_state;

#define V34_DEFERRED_RX_PACKETS 16u

typedef struct {
    bool call_modem;
    uint8_t md_length_35ms;
    v34_symbol_rate symbol_rate;
    unsigned maximum_rate;
    v34_trellis_kind trellis;
    bool expanded_shaping;
    unsigned sample_rate;
    double training_amplitude;
    double coordinate_scale;
    v34_mp0 mp;
    bool directional_symbols;
    v34_symbol_rate tx_symbol_rate;
    v34_symbol_rate rx_symbol_rate;
    bool tx_high_carrier;
    bool rx_high_carrier;
} v34_session_config;

typedef struct {
    v34_session_config config;
    v34_phase3_stream phase3_tx;
    v34_phase3_receiver phase3_rx;
    v34_phase4_stream phase4_tx;
    v34_phase4_receiver phase4_rx;
    v34_b1_data_link data_link;
    v34_final_rates rates;
    uint8_t pending[V34_UART_QUEUE_SIZE];
    size_t pending_head;
    size_t pending_tail;
    uint8_t deferred_rx[V34_DEFERRED_RX_PACKETS][V34_PCMA_PACKET_SAMPLES];
    unsigned deferred_rx_count;
    v34_session_state state;
    bool phase4_tx_ready;
    bool phase4_rx_ready;
} v34_session;

bool v34_session_init(v34_session *session,
                      const v34_session_config *config);
bool v34_session_generate(v34_session *session,
                          uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
bool v34_session_receive(v34_session *session,
                         const uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
size_t v34_session_write(v34_session *session,
                         const uint8_t *bytes, size_t count);
size_t v34_session_read(v34_session *session,
                        uint8_t *bytes, size_t capacity);
bool v34_session_connected(const v34_session *session);
v34_session_state v34_session_get_state(const v34_session *session);

#endif
