#include "v34_modem_session.h"

#include "pcma.h"

#include <string.h>

#define MODEM_QUEUE_MASK (V34_UART_QUEUE_SIZE - 1u)

static bool quiet_packet(const uint8_t *pcma)
{
    size_t i;
    for (i = 0; i < V34_PCMA_PACKET_SAMPLES; ++i) {
        int sample = pcma_decode(pcma[i]);
        if (sample < -32 || sample > 32)
            return false;
    }
    return true;
}

static bool start_training(v34_modem_session *s)
{
    const v34_info1a *info1a;
    v34_session_config config;
    v34_mode mode;
    bool call_high, answer_high;
    unsigned call_rate, answer_rate;

    if (!v34_phase2_session_mode(&s->phase2, &mode,
                                 &call_high, &answer_high))
        return false;
    info1a = v34_phase2_session_info1a(&s->phase2);
    if (info1a == NULL)
        return false;
    call_rate = s->config.role == V34_INFO_CALL_MODEM ?
                mode.tx_rate : mode.rx_rate;
    answer_rate = s->config.role == V34_INFO_CALL_MODEM ?
                  mode.rx_rate : mode.tx_rate;

    memset(&config, 0, sizeof(config));
    config.call_modem = s->config.role == V34_INFO_CALL_MODEM;
    config.md_length_35ms = info1a->md_length_35ms;
    config.symbol_rate = mode.tx_symbol;
    config.maximum_rate = s->config.maximum_rate;
    config.trellis = s->config.trellis;
    config.expanded_shaping = s->config.expanded_shaping;
    config.sample_rate = s->config.sample_rate;
    config.training_amplitude = s->config.training_amplitude;
    config.coordinate_scale = s->config.coordinate_scale;
    config.mp.call_to_answer_rate_2400 =
        (uint8_t)(call_rate / V34_RATE_STEP);
    config.mp.answer_to_call_rate_2400 =
        (uint8_t)(answer_rate / V34_RATE_STEP);
    config.mp.trellis_encoder = (uint8_t)s->config.trellis;
    config.mp.expanded_shaping = s->config.expanded_shaping;
    config.mp.rate_mask = s->config.allowed_rates;
    config.mp.asymmetric_rates = true;
    config.directional_symbols = true;
    config.tx_symbol_rate = mode.tx_symbol;
    config.rx_symbol_rate = mode.rx_symbol;
    config.tx_high_carrier = config.call_modem ? call_high : answer_high;
    config.rx_high_carrier = config.call_modem ? answer_high : call_high;
    if (!v34_session_init(&s->training, &config))
        return false;
    while (s->pending_head != s->pending_tail) {
        size_t end = s->pending_tail > s->pending_head ?
                     s->pending_tail : V34_UART_QUEUE_SIZE;
        size_t count = v34_session_write(&s->training,
            s->pending + s->pending_head, end - s->pending_head);
        if (count == 0u)
            return false;
        s->pending_head = (s->pending_head + count) & MODEM_QUEUE_MASK;
    }
    s->state = V34_MODEM_SESSION_TRAINING_DATA;
    return true;
}

bool v34_modem_session_init(v34_modem_session *s,
                            const v34_modem_session_config *config)
{
    if (s == NULL || config == NULL || config->sample_rate == 0u ||
        config->trellis > V34_TRELLIS_64)
        return false;
    memset(s, 0, sizeof(*s));
    s->config = *config;
    if (!v34_phase2_session_init(&s->phase2, config->role, &config->info0,
            config->allowed_symbols, config->allowed_rates,
            config->maximum_rate, config->maximum_symbol_difference))
        return false;
    s->state = V34_MODEM_SESSION_PHASE2;
    return true;
}

bool v34_modem_session_generate(v34_modem_session *s,
                                uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    if (s == NULL || pcma == NULL || s->state == V34_MODEM_SESSION_FAILED)
        return false;
    if (s->state == V34_MODEM_SESSION_PHASE2) {
        v34_phase2_session_generate(&s->phase2, pcma,
                                    V34_PCMA_PACKET_SAMPLES);
        if (v34_phase2_session_complete(&s->phase2) && !start_training(s)) {
            s->state = V34_MODEM_SESSION_FAILED;
            return false;
        }
        return true;
    }
    if (v34_session_generate(&s->training, pcma))
        return true;
    s->state = V34_MODEM_SESSION_FAILED;
    return false;
}

bool v34_modem_session_receive(v34_modem_session *s,
                               const uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    if (s == NULL || pcma == NULL || s->state == V34_MODEM_SESSION_FAILED)
        return false;
    if (s->state == V34_MODEM_SESSION_PHASE2) {
        if (!s->phase2_rx_ready) {
            bool silence = quiet_packet(pcma);
            if (silence) {
                if (++s->phase2_silence_packets >= 2u)
                    s->phase2_rx_ready = true;
            } else {
                s->phase2_silence_packets = 0u;
            }
            return true;
        }
        v34_phase2_session_receive(&s->phase2, pcma,
                                   V34_PCMA_PACKET_SAMPLES);
        if (s->phase2.state == V34_PHASE2_SESSION_FAILED ||
            (v34_phase2_session_complete(&s->phase2) && !start_training(s))) {
            s->state = V34_MODEM_SESSION_FAILED;
            return false;
        }
        return true;
    }
    if (!s->training_rx_started) {
        v34_phase3_receiver candidate = s->training.phase3_rx;
        if (s->training.config.call_modem) {
            if (quiet_packet(pcma)) {
                ++s->training_silence_packets;
                return true;
            }
            if (s->training_silence_packets < 3u) {
                s->training_silence_packets = 0u;
                return true;
            }
            {
                uint8_t silence[V34_PCMA_PACKET_SAMPLES];
                unsigned packet;
                memset(silence, 0xd5, sizeof(silence));
                for (packet = 0; packet < 3u; ++packet) {
                    if (v34_phase3_receiver_feed(&candidate,
                            silence, sizeof(silence)) != sizeof(silence)) {
                        s->training_silence_packets = 0u;
                        return true;
                    }
                }
            }
        }
        if (v34_phase3_receiver_feed(&candidate, pcma,
                                     V34_PCMA_PACKET_SAMPLES) !=
                V34_PCMA_PACKET_SAMPLES ||
            v34_phase3_receiver_failed(&candidate)) {
            s->training_silence_packets = 0u;
            return true;
        }
        s->training.phase3_rx = candidate;
        s->training_rx_started = true;
        return true;
    }
    if (v34_session_receive(&s->training, pcma))
        return true;
    s->state = V34_MODEM_SESSION_FAILED;
    return false;
}

size_t v34_modem_session_write(v34_modem_session *s,
                               const uint8_t *bytes, size_t count)
{
    size_t written = 0u;
    if (s == NULL || (bytes == NULL && count != 0u) ||
        s->state == V34_MODEM_SESSION_FAILED)
        return 0u;
    if (s->state == V34_MODEM_SESSION_TRAINING_DATA)
        return v34_session_write(&s->training, bytes, count);
    while (written < count) {
        size_t next = (s->pending_tail + 1u) & MODEM_QUEUE_MASK;
        if (next == s->pending_head)
            break;
        s->pending[s->pending_tail] = bytes[written++];
        s->pending_tail = next;
    }
    return written;
}

size_t v34_modem_session_read(v34_modem_session *s,
                              uint8_t *bytes, size_t capacity)
{
    return s != NULL && s->state == V34_MODEM_SESSION_TRAINING_DATA ?
        v34_session_read(&s->training, bytes, capacity) : 0u;
}

bool v34_modem_session_connected(const v34_modem_session *s)
{
    return s != NULL && s->state == V34_MODEM_SESSION_TRAINING_DATA &&
           v34_session_connected(&s->training);
}

bool v34_modem_session_mode(const v34_modem_session *s, v34_mode *mode)
{
    return s != NULL && mode != NULL &&
        v34_phase2_session_mode(&s->phase2, mode, NULL, NULL);
}
