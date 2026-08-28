#ifndef SOFTMODEM_V34_MODEM_SESSION_H
#define SOFTMODEM_V34_MODEM_SESSION_H

#include "v34_phase2_session.h"
#include "v34_session.h"

typedef enum {
    V34_MODEM_SESSION_PHASE2 = 0,
    V34_MODEM_SESSION_TRAINING_DATA,
    V34_MODEM_SESSION_FAILED
} v34_modem_session_state;

typedef struct {
    v34_info_modem_role role;
    v34_info0 info0;
    uint8_t allowed_symbols;
    uint16_t allowed_rates;
    unsigned maximum_rate;
    unsigned maximum_symbol_difference;
    v34_trellis_kind trellis;
    bool expanded_shaping;
    unsigned sample_rate;
    double training_amplitude;
    double coordinate_scale;
} v34_modem_session_config;

typedef struct {
    v34_modem_session_config config;
    v34_phase2_session phase2;
    v34_session training;
    uint8_t pending[V34_UART_QUEUE_SIZE];
    size_t pending_head;
    size_t pending_tail;
    v34_modem_session_state state;
    bool training_rx_started;
} v34_modem_session;

bool v34_modem_session_init(v34_modem_session *session,
                            const v34_modem_session_config *config);
bool v34_modem_session_generate(v34_modem_session *session,
                                uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
bool v34_modem_session_receive(v34_modem_session *session,
                               const uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
size_t v34_modem_session_write(v34_modem_session *session,
                               const uint8_t *bytes, size_t count);
size_t v34_modem_session_read(v34_modem_session *session,
                              uint8_t *bytes, size_t capacity);
bool v34_modem_session_connected(const v34_modem_session *session);
bool v34_modem_session_mode(const v34_modem_session *session,
                            v34_mode *mode);

#endif
