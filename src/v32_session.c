#include "v32_session.h"

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
            } else if (!s->startup.selected_rate) {
                if (!s->rate_tx_ready) {
                    v32_rate_tx_init(&s->rate_tx, s->role,
                                     s->startup.local_rate_word, s->last_tx);
                    s->rate_tx_ready = 1;
                }
                states[i] = v32_rate_tx_next(&s->rate_tx);
            } else if (!s->remote_e) {
                if (!s->e_tx_ready) {
                    v32_rate_tx_init(&s->e_tx, s->role,
                                     v32_std_e_word(s->startup.selected_rate, 0),
                                     s->last_tx);
                    s->e_tx_ready = 1;
                }
                states[i] = v32_rate_tx_next(&s->e_tx);
            } else {
                states[i] = v32_data_next_4800(&s->data);
                if (s->tx_marking < 128) s->tx_marking++;
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
    if (s->data_ready || !s->startup.selected_rate || !s->remote_e) return;
    /* E is the common differential reference at the data boundary.  The
     * composite session currently represents that boundary as state A. */
    v32_data_init(&s->data, s->role, V32_STATE_A, V32_STATE_A);
    v32_qam_init(&s->qam);
    s->data_ready = 1;
}

void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count)
{
    begin_data(s);
    if (s->data_ready && s->startup.selected_rate == 9600) {
            uint8_t symbols[64]; size_t n = count * 2400 / 8000;
            for (size_t i = 0; i < n; i++) {
                symbols[i] = v32_data_next_9600(&s->data);
                if (s->tx_marking < 128) s->tx_marking++;
            }
            (void)v32_qam_write(&s->qam, symbols, n);
            v32_qam_generate(&s->qam, pcm, count);
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
            if (!s->startup.selected_rate) {
                uint16_t word;
                if (v32_rate_rx_put(&s->rate_rx, states[i], &word)) {
                    int selected = v32_startup_rate_word(&s->startup, word);
                    if (selected > 0) {
                        v32_e_rx_init(&s->e_rx, other(s->role), states[i]);
                        s->e_rx_ready = 1;
                    }
                }
            } else if (!s->remote_e && s->e_rx_ready) {
                int rate, trellis;
                if (v32_e_rx_put(&s->e_rx, states[i], &rate, &trellis) &&
                    rate == s->startup.selected_rate && !trellis) {
                    s->remote_e = 1;
                    begin_data(s);
                    /* The peer changes from E to marking on its next media
                     * block.  Discard the tail of this already received E
                     * block so it cannot advance the data descrambler. */
                    return;
                }
            } else if (s->data_ready && s->startup.selected_rate == 4800) {
                v32_data_put_4800(&s->data, states[i]);
                if (s->rx_marking < 128 && ++s->rx_marking == 128)
                    s->data.rh = s->data.rt;
            }
        }
    }
}

void v32_session_receive(struct v32_session *s, const int16_t *pcm, size_t count)
{
    begin_data(s);
    if (s->data_ready && s->startup.selected_rate == 9600) {
        uint8_t symbols[128];
        v32_qam_receive(&s->qam, pcm, count);
        size_t n = v32_qam_read(&s->qam, symbols, 128);
        for (size_t i = 0; i < n; i++) {
            v32_data_put_9600(&s->data, symbols[i]);
            if (s->rx_marking < 128 && ++s->rx_marking == 128)
                s->data.rh = s->data.rt;
        }
    } else {
        v32_line_receive(&s->line, pcm, count);
        consume_line(s);
    }
    s->rx_samples += count;
}

size_t v32_session_write(struct v32_session *s, const uint8_t *b, size_t n)
{ return s->data_ready ? v32_data_write(&s->data, b, n) : 0; }
size_t v32_session_read(struct v32_session *s, uint8_t *b, size_t n)
{ return s->data_ready ? v32_data_read(&s->data, b, n) : 0; }
int v32_session_connected(const struct v32_session *s)
{ return s->data_ready && s->tx_marking >= 128 && s->rx_marking >= 128; }
int v32_session_rate(const struct v32_session *s) { return s->startup.selected_rate; }
