#include "v34_phase2_session.h"

#include <string.h>

#define ANSWER_A_BRIDGE_SAMPLES 80u

bool v34_phase2_session_init(v34_phase2_session *session,
                             v34_info_modem_role role,
                             const v34_info0 *local_info0,
                             uint8_t allowed_symbols,
                             uint16_t allowed_rates,
                             unsigned maximum_rate,
                             unsigned maximum_symbol_difference)
{
    if (session == NULL || maximum_symbol_difference > 5u)
        return false;
    memset(session, 0, sizeof(*session));
    session->role = role;
    session->state = V34_PHASE2_SESSION_RANGING;
    session->allowed_symbols = allowed_symbols;
    session->allowed_rates = allowed_rates;
    session->maximum_rate = maximum_rate;
    session->maximum_symbol_difference = maximum_symbol_difference;
    return v34_phase2_ranging_init(&session->ranging, role, local_info0);
}

static bool start_probing(v34_phase2_session *session)
{
    unsigned rtd = v34_phase2_ranging_round_trip_samples(&session->ranging);
    if (!v34_phase2_probe_exchange_init(&session->probing, session->role,
            session->allowed_symbols, session->allowed_rates,
            session->maximum_rate, session->maximum_symbol_difference, rtd)) {
        session->state = V34_PHASE2_SESSION_FAILED;
        return false;
    }
    session->state = V34_PHASE2_SESSION_PROBING;
    return true;
}

static void update_ranging_transition(v34_phase2_session *session)
{
    if (!v34_phase2_ranging_complete(&session->ranging))
        return;
    if (session->role == V34_INFO_ANSWER_MODEM) {
        session->state = V34_PHASE2_SESSION_ANSWER_BRIDGE;
        session->bridge_samples = ANSWER_A_BRIDGE_SAMPLES;
    } else {
        (void)start_probing(session);
    }
}

static void generate_one(v34_phase2_session *session, uint8_t *sample)
{
    switch (session->state) {
    case V34_PHASE2_SESSION_RANGING:
        v34_phase2_ranging_generate(&session->ranging, sample, 1u);
        update_ranging_transition(session);
        break;
    case V34_PHASE2_SESSION_ANSWER_BRIDGE:
        v34_phase2_ranging_generate(&session->ranging, sample, 1u);
        if (--session->bridge_samples == 0u)
            (void)start_probing(session);
        break;
    case V34_PHASE2_SESSION_PROBING:
        v34_phase2_probe_exchange_generate(&session->probing, sample, 1u);
        if (v34_phase2_probe_exchange_complete(&session->probing))
            session->state = V34_PHASE2_SESSION_COMPLETE;
        break;
    default:
        *sample = 0xd5u;
        break;
    }
}

void v34_phase2_session_generate(v34_phase2_session *session, uint8_t *pcma,
                                 size_t sample_count)
{
    size_t i;
    if (session == NULL || pcma == NULL)
        return;
    for (i = 0; i < sample_count; ++i)
        generate_one(session, &pcma[i]);
}

static void receive_one(v34_phase2_session *session, uint8_t sample)
{
    switch (session->state) {
    case V34_PHASE2_SESSION_RANGING:
        v34_phase2_ranging_receive(&session->ranging, &sample, 1u);
        update_ranging_transition(session);
        break;
    case V34_PHASE2_SESSION_ANSWER_BRIDGE:
        v34_phase2_ranging_receive(&session->ranging, &sample, 1u);
        break;
    case V34_PHASE2_SESSION_PROBING:
        v34_phase2_probe_exchange_receive(&session->probing, &sample, 1u);
        if (session->probing.rx_state == V34_PHASE2_PROBE_RX_FAILED)
            session->state = V34_PHASE2_SESSION_FAILED;
        else if (v34_phase2_probe_exchange_complete(&session->probing))
            session->state = V34_PHASE2_SESSION_COMPLETE;
        break;
    default:
        break;
    }
}

void v34_phase2_session_receive(v34_phase2_session *session,
                                const uint8_t *pcma, size_t sample_count)
{
    size_t i;
    if (session == NULL || pcma == NULL ||
        session->state == V34_PHASE2_SESSION_FAILED)
        return;
    for (i = 0; i < sample_count; ++i)
        receive_one(session, pcma[i]);
}

bool v34_phase2_session_complete(const v34_phase2_session *session)
{
    return session != NULL && session->state == V34_PHASE2_SESSION_COMPLETE;
}

const v34_info1a *v34_phase2_session_info1a(
    const v34_phase2_session *session)
{
    return session != NULL &&
           (session->state == V34_PHASE2_SESSION_COMPLETE ||
            session->state == V34_PHASE2_SESSION_PROBING) ?
        v34_phase2_probe_exchange_info1a(&session->probing) : NULL;
}

unsigned v34_phase2_session_round_trip_samples(
    const v34_phase2_session *session)
{
    return session == NULL ? 0u :
        v34_phase2_ranging_round_trip_samples(&session->ranging);
}
