#include "v32_session.h"

#define START_DATA_SAMPLES 9600u /* 1.2 s after V.8 selection */

static enum v32_std_role other(enum v32_std_role r)
{
    return r == V32_STD_CALL ? V32_STD_ANSWER : V32_STD_CALL;
}

void v32_session_init(struct v32_session *s, enum v32_std_role role,
                      int allow_4800, int allow_9600)
{
    *s = (struct v32_session){0};
    s->role = role;
    s->last_tx = s->last_rx = V32_STATE_A;
    v32_line_init(&s->line);
    v32_qam_init(&s->qam);
    v32_training_init(&s->training, role);
    v32_startup_init(&s->startup, role, allow_4800, allow_9600);
    s->startup.phase = V32_START_RATE_1;
}

static void queue_line_symbols(struct v32_session *s, size_t count)
{
    enum v32_carrier_state states[64];
    while (count) {
        size_t n = count > 64 ? 64 : count;
        for (size_t i = 0; i < n; i++) {
            if (s->tx_symbols < 1552) {
                states[i] = v32_training_next(&s->training);
            } else {
                if (!s->rate_tx_ready) {
                    v32_rate_tx_init(&s->rate_tx, s->role,
                                     s->startup.local_rate_word, s->last_tx);
                    s->rate_tx_ready = 1;
                }
                states[i] = v32_rate_tx_next(&s->rate_tx);
            }
            s->last_tx = states[i];
            s->tx_symbols++;
        }
        (void)v32_line_write(&s->line, states, n);
        count -= n;
    }
}

static void begin_data(struct v32_session *s)
{
    if (s->data_ready || !s->startup.selected_rate) return;
    /* E is the common differential reference at the data boundary.  The
     * composite session currently represents that boundary as state A. */
    v32_data_init(&s->data, s->role, V32_STATE_A, V32_STATE_A);
    v32_qam_init(&s->qam);
    s->data_ready = 1;
}

void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count)
{
    if (s->tx_samples >= START_DATA_SAMPLES) begin_data(s);
    if (s->data_ready) {
        if (s->startup.selected_rate == 9600) {
            uint8_t symbols[64]; size_t n = count * 2400 / 8000;
            for (size_t i = 0; i < n; i++) symbols[i] = v32_data_next_9600(&s->data);
            (void)v32_qam_write(&s->qam, symbols, n);
            v32_qam_generate(&s->qam, pcm, count);
        } else {
            enum v32_carrier_state states[64]; size_t n = count * 2400 / 8000;
            for (size_t i = 0; i < n; i++) states[i] = v32_data_next_4800(&s->data);
            (void)v32_line_write(&s->line, states, n);
            v32_line_generate(&s->line, pcm, count);
        }
    } else {
        queue_line_symbols(s, count * 2400 / 8000);
        v32_line_generate(&s->line, pcm, count);
    }
    s->tx_samples += count;
}

static void consume_line(struct v32_session *s)
{
    enum v32_carrier_state states[128]; size_t n;
    while ((n = v32_line_read(&s->line, states, 128)) != 0) {
        for (size_t i = 0; i < n; i++) {
            if (s->rx_symbols < 1552) {
                s->last_rx = states[i];
                s->rx_symbols++;
                continue;
            }
            if (!s->rate_rx_ready) {
                v32_rate_rx_init(&s->rate_rx, other(s->role), s->last_rx);
                s->rate_rx_ready = 1;
            }
            s->last_rx = states[i];
            s->rx_symbols++;
            uint16_t word;
            if (v32_rate_rx_put(&s->rate_rx, states[i], &word))
                (void)v32_startup_rate_word(&s->startup, word);
        }
    }
}

void v32_session_receive(struct v32_session *s, const int16_t *pcm, size_t count)
{
    if (s->rx_samples >= START_DATA_SAMPLES) begin_data(s);
    if (s->data_ready && s->startup.selected_rate == 9600) {
        uint8_t symbols[128];
        v32_qam_receive(&s->qam, pcm, count);
        size_t n = v32_qam_read(&s->qam, symbols, 128);
        for (size_t i = 0; i < n; i++) v32_data_put_9600(&s->data, symbols[i]);
    } else {
        enum v32_carrier_state states[128];
        v32_line_receive(&s->line, pcm, count);
        if (!s->data_ready) consume_line(s);
        else {
            size_t n;
            while ((n = v32_line_read(&s->line, states, 128)) != 0)
                for (size_t i = 0; i < n; i++) v32_data_put_4800(&s->data, states[i]);
        }
    }
    s->rx_samples += count;
}

size_t v32_session_write(struct v32_session *s, const uint8_t *b, size_t n)
{ return s->data_ready ? v32_data_write(&s->data, b, n) : 0; }
size_t v32_session_read(struct v32_session *s, uint8_t *b, size_t n)
{ return s->data_ready ? v32_data_read(&s->data, b, n) : 0; }
int v32_session_connected(const struct v32_session *s) { return s->data_ready; }
int v32_session_rate(const struct v32_session *s) { return s->startup.selected_rate; }
