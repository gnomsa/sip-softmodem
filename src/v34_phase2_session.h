#ifndef SOFTMODEM_V34_PHASE2_SESSION_H
#define SOFTMODEM_V34_PHASE2_SESSION_H

#include "v34_caps.h"
#include "v34_info.h"
#include "v34_info_modem.h"
#include "v34_phase2_probe_exchange.h"
#include "v34_phase2_ranging.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    V34_PHASE2_SESSION_RANGING = 0,
    V34_PHASE2_SESSION_ANSWER_BRIDGE,
    V34_PHASE2_SESSION_PROBING,
    V34_PHASE2_SESSION_COMPLETE,
    V34_PHASE2_SESSION_FAILED
} v34_phase2_session_state;

typedef struct {
    v34_info_modem_role role;
    v34_phase2_session_state state;
    uint8_t allowed_symbols;
    uint16_t allowed_rates;
    unsigned maximum_rate;
    unsigned maximum_symbol_difference;
    unsigned bridge_samples;
    v34_phase2_ranging ranging;
    v34_phase2_probe_exchange probing;
} v34_phase2_session;

bool v34_phase2_session_init(v34_phase2_session *session,
                             v34_info_modem_role role,
                             const v34_info0 *local_info0,
                             uint8_t allowed_symbols,
                             uint16_t allowed_rates,
                             unsigned maximum_rate,
                             unsigned maximum_symbol_difference);
void v34_phase2_session_generate(v34_phase2_session *session, uint8_t *pcma,
                                 size_t sample_count);
void v34_phase2_session_receive(v34_phase2_session *session,
                                const uint8_t *pcma, size_t sample_count);
bool v34_phase2_session_complete(const v34_phase2_session *session);
const v34_info1a *v34_phase2_session_info1a(
    const v34_phase2_session *session);
unsigned v34_phase2_session_round_trip_samples(
    const v34_phase2_session *session);
bool v34_phase2_session_mode(const v34_phase2_session *session,
                             v34_mode *mode,
                             bool *call_high_carrier,
                             bool *answer_high_carrier);

#endif
