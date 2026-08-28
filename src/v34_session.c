#include "v34_session.h"

#include <string.h>

#define SESSION_QUEUE_MASK (V34_UART_QUEUE_SIZE - 1u)

static v34_symbol_rate tx_symbol(const v34_session *s)
{
    return s->config.directional_symbols ? s->config.tx_symbol_rate :
                                           s->config.symbol_rate;
}

static v34_symbol_rate rx_symbol(const v34_session *s)
{
    return s->config.directional_symbols ? s->config.rx_symbol_rate :
                                           s->config.symbol_rate;
}

static bool tx_high_carrier(const v34_session *s)
{
    return s->config.directional_symbols ? s->config.tx_high_carrier :
                                           s->config.call_modem;
}

static bool rx_high_carrier(const v34_session *s)
{
    return s->config.directional_symbols ? s->config.rx_high_carrier :
                                           !s->config.call_modem;
}

static bool start_phase4_tx(v34_session *s)
{
    bool call = s->config.call_modem;
    if (!v34_phase4_stream_init(
            &s->phase4_tx, call, &s->phase3_tx.scrambler,
            s->phase3_tx.j_rotation, &s->config.mp,
            tx_symbol(s), tx_high_carrier(s), s->config.sample_rate,
            s->config.training_amplitude))
        return false;
    s->phase4_tx_ready = true;
    if (s->phase4_rx_ready)
        s->state = V34_SESSION_PHASE4;
    return true;
}

static bool start_phase4_rx(v34_session *s)
{
    bool call = s->config.call_modem;
    if (!v34_phase4_receiver_init(
            &s->phase4_rx, !call, &s->phase3_rx.scrambler,
            s->phase3_rx.trn_rotation, rx_symbol(s), rx_high_carrier(s),
            s->config.sample_rate))
        return false;
    s->phase4_rx_ready = true;
    if (s->phase4_tx_ready)
        s->state = V34_SESSION_PHASE4;
    return true;
}

static bool finish_phase3_tx_if_ready(v34_session *s)
{
    const v34_phase3_event *tx_event;
    if (s->state != V34_SESSION_PHASE3 ||
        !v34_phase3_receiver_j_detected(&s->phase3_rx))
        return true;
    if (!s->phase4_tx_ready) {
        tx_event = v34_phase3_current(&s->phase3_tx.cursor);
        if (tx_event == NULL || tx_event->signal != V34_P3_J)
            return true;
        if (!v34_phase3_stream_finish_j(&s->phase3_tx) ||
            !start_phase4_tx(s))
            return false;
    }
    return true;
}

static bool start_data_link(v34_session *s)
{
    unsigned tx_rate;
    unsigned rx_rate;
    if (s->state != V34_SESSION_PHASE4 ||
        !v34_phase4_stream_complete(&s->phase4_tx) ||
        !v34_phase4_receiver_complete(&s->phase4_rx))
        return true;
    if (!v34_mp0_negotiate_rates(&s->config.mp, &s->phase4_rx.mp_prime,
                                 s->config.maximum_rate, &s->rates))
        return false;
    tx_rate = s->config.call_modem ? s->rates.call_to_answer
                                   : s->rates.answer_to_call;
    rx_rate = s->config.call_modem ? s->rates.answer_to_call
                                   : s->rates.call_to_answer;
    if (!v34_b1_data_link_init_after_phase4(
            &s->data_link, s->config.call_modem,
            &s->phase4_tx, &s->phase4_rx,
            tx_symbol(s), tx_rate,
            rx_symbol(s), rx_rate,
            s->config.trellis, s->config.expanded_shaping,
            s->config.sample_rate, s->config.coordinate_scale))
        return false;
    while (s->pending_head != s->pending_tail) {
        size_t end = s->pending_tail > s->pending_head
                         ? s->pending_tail : V34_UART_QUEUE_SIZE;
        size_t n = v34_b1_data_link_write(
            &s->data_link, s->pending + s->pending_head,
            end - s->pending_head);
        if (n == 0u)
            return false;
        s->pending_head = (s->pending_head + n) & SESSION_QUEUE_MASK;
    }
    s->state = V34_SESSION_B1_DATA;
    return true;
}

static bool fail(v34_session *s)
{
    if (s != NULL)
        s->state = V34_SESSION_FAILED;
    return false;
}

bool v34_session_init(v34_session *s, const v34_session_config *config)
{
    v34_phase3_role local_role;
    v34_phase3_role remote_role;
    if (s == NULL || config == NULL || config->sample_rate == 0u ||
        config->maximum_rate < V34_RATE_MIN ||
        config->maximum_rate > V34_RATE_MAX)
        return false;
    memset(s, 0, sizeof(*s));
    s->config = *config;
    local_role = config->call_modem ? V34_PHASE3_CALL : V34_PHASE3_ANSWER;
    remote_role = config->call_modem ? V34_PHASE3_ANSWER : V34_PHASE3_CALL;
    if (!v34_phase3_stream_init(
            &s->phase3_tx, local_role, config->md_length_35ms,
            tx_symbol(s), tx_high_carrier(s), config->sample_rate,
            config->training_amplitude) ||
        !v34_phase3_receiver_init(
            &s->phase3_rx, remote_role, config->md_length_35ms,
            rx_symbol(s), rx_high_carrier(s),
            config->sample_rate))
        return false;
    s->state = V34_SESSION_PHASE3;
    return true;
}

bool v34_session_generate(v34_session *s,
                          uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    if (s == NULL || pcma == NULL || s->state == V34_SESSION_FAILED)
        return false;
    if (s->state == V34_SESSION_PHASE3 && !s->phase4_tx_ready) {
        if (v34_phase3_stream_generate(
                &s->phase3_tx, pcma, V34_PCMA_PACKET_SAMPLES) !=
            V34_PCMA_PACKET_SAMPLES || !finish_phase3_tx_if_ready(s))
            return fail(s);
        return true;
    }
    if ((s->state == V34_SESSION_PHASE3 && s->phase4_tx_ready) ||
        s->state == V34_SESSION_PHASE4) {
        if (v34_phase4_stream_generate(
                &s->phase4_tx, pcma, V34_PCMA_PACKET_SAMPLES) !=
            V34_PCMA_PACKET_SAMPLES)
            return fail(s);
        return true;
    }
    if (s->state == V34_SESSION_B1_DATA &&
        v34_b1_data_link_generate(&s->data_link, pcma))
        return true;
    return fail(s);
}

bool v34_session_receive(v34_session *s,
                         const uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    size_t sample;
    if (s == NULL || pcma == NULL || s->state == V34_SESSION_FAILED)
        return false;
    if (s->state == V34_SESSION_PHASE3 && !s->phase4_rx_ready) {
        v34_phase3_receiver saved = s->phase3_rx;
        bool watching_j = v34_phase3_receiver_j_detected(&saved);
        if (v34_phase3_receiver_feed(
                &s->phase3_rx, pcma, V34_PCMA_PACKET_SAMPLES) !=
            V34_PCMA_PACKET_SAMPLES || !finish_phase3_tx_if_ready(s))
            return fail(s);
        if (watching_j && v34_phase3_receiver_j_ended(&s->phase3_rx)) {
            s->phase3_rx = saved;
            if (!v34_phase3_receiver_finish_j(&s->phase3_rx) ||
                !start_phase4_rx(s))
                return fail(s);
            for (sample = 0; sample < V34_PCMA_PACKET_SAMPLES; ++sample)
                if (!v34_phase4_receiver_feed(&s->phase4_rx, pcma[sample]))
                    return fail(s);
        }
        return true;
    }
    if ((s->state == V34_SESSION_PHASE3 && s->phase4_rx_ready) ||
        s->state == V34_SESSION_PHASE4) {
        for (sample = 0; sample < V34_PCMA_PACKET_SAMPLES; ++sample)
            if (!v34_phase4_receiver_feed(&s->phase4_rx, pcma[sample]))
                return fail(s);
        if (!start_data_link(s))
            return fail(s);
        return true;
    }
    if (s->state == V34_SESSION_B1_DATA &&
        v34_b1_data_link_receive(&s->data_link, pcma))
        return true;
    return fail(s);
}

size_t v34_session_write(v34_session *s,
                         const uint8_t *bytes, size_t count)
{
    size_t written = 0;
    if (s == NULL || (bytes == NULL && count != 0u) ||
        s->state == V34_SESSION_FAILED)
        return 0;
    if (s->state == V34_SESSION_B1_DATA)
        return v34_b1_data_link_write(&s->data_link, bytes, count);
    while (written < count) {
        size_t next = (s->pending_tail + 1u) & SESSION_QUEUE_MASK;
        if (next == s->pending_head)
            break;
        s->pending[s->pending_tail] = bytes[written++];
        s->pending_tail = next;
    }
    return written;
}

size_t v34_session_read(v34_session *s, uint8_t *bytes, size_t capacity)
{
    if (s == NULL || s->state != V34_SESSION_B1_DATA)
        return 0;
    return v34_b1_data_link_read(&s->data_link, bytes, capacity);
}

bool v34_session_connected(const v34_session *s)
{
    return s != NULL && s->state == V34_SESSION_B1_DATA &&
           v34_b1_data_link_connected(&s->data_link);
}

v34_session_state v34_session_get_state(const v34_session *s)
{
    return s != NULL ? s->state : V34_SESSION_FAILED;
}
