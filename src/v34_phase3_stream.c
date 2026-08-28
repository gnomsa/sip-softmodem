#include "v34_phase3_stream.h"

#include "pcma.h"

#include <string.h>

static bool set_symbol_phase(v34_phase3_stream *s,
                             const v34_phase3_event *event)
{
    uint8_t phase = 0;

    switch (event->signal) {
    case V34_P3_S:
        if (!v34_s_phase(s->event_symbol, false, &phase)) return false;
        break;
    case V34_P3_S_BAR:
        if (!v34_s_phase(s->event_symbol, true, &phase)) return false;
        break;
    case V34_P3_PP:
        if (!v34_pp_phase(s->event_symbol, &phase)) return false;
        break;
    case V34_P3_TRN:
        if (!v34_trn4_phase(&s->scrambler, &phase)) return false;
        break;
    case V34_P3_J:
        if (!v34_j4_phase(&s->scrambler, &s->j_bit_index,
                          &s->j_rotation, &phase)) return false;
        break;
    default:
        return false;
    }
    v34_training_tx_set_phase(&s->tx, phase);
    return true;
}

static void enter_event(v34_phase3_stream *s)
{
    const v34_phase3_event *event = v34_phase3_current(&s->cursor);
    s->event_samples = 0;
    s->event_symbol = 0;
    if (event != NULL && event->signal == V34_P3_TRN)
        v34_scrambler_init(&s->scrambler, s->role == V34_PHASE3_CALL);
    if (event != NULL && event->signal == V34_P3_J) {
        s->j_bit_index = 0;
        s->j_rotation = ((12u - s->tx.symbol_phase) % 12u) / 3u;
        (void)set_symbol_phase(s, event);
    } else if (event != NULL && event->symbols != 0)
        (void)set_symbol_phase(s, event);
}

bool v34_phase3_stream_init(v34_phase3_stream *s, v34_phase3_role role,
                            uint8_t md_length_35ms, v34_symbol_rate rate,
                            bool high_carrier, unsigned sample_rate,
                            double amplitude)
{
    if (s == NULL)
        return false;
    memset(s, 0, sizeof(*s));
    s->role = role;
    s->sample_rate = sample_rate;
    if (!v34_phase3_build_tx_plan(role, md_length_35ms, &s->plan) ||
        !v34_phase3_cursor_init(&s->cursor, &s->plan) ||
        !v34_symbol_clock_init(&s->clock, rate, sample_rate) ||
        !v34_training_tx_init(&s->tx, rate, high_carrier, sample_rate, amplitude))
        return false;
    v34_scrambler_init(&s->scrambler, role == V34_PHASE3_CALL);
    enter_event(s);
    return true;
}

size_t v34_phase3_stream_generate(v34_phase3_stream *s, uint8_t *pcma,
                                  size_t count)
{
    size_t i;
    if (s == NULL || pcma == NULL)
        return 0;
    for (i = 0; i < count; ++i) {
        const v34_phase3_event *event = v34_phase3_current(&s->cursor);
        if (event == NULL) {
            pcma[i] = pcma_encode(0);
            continue;
        }
        if (event->milliseconds != 0) {
            uint32_t target = event->milliseconds * s->sample_rate / 1000u;
            pcma[i] = pcma_encode(0);
            s->event_samples++;
            if (s->event_samples == target) {
                (void)v34_phase3_advance(&s->cursor, event->milliseconds);
                enter_event(s);
            }
            continue;
        }
        pcma[i] = v34_training_tx_pcma(&s->tx);
        s->active_samples++;
        if (v34_symbol_clock_tick(&s->clock)) {
            if (event->signal != V34_P3_J)
                (void)v34_phase3_advance(&s->cursor, 1);
            s->event_symbol++;
            if (v34_phase3_current(&s->cursor) != event)
                enter_event(s);
            else
                (void)set_symbol_phase(s, event);
        }
    }
    return count;
}

bool v34_phase3_stream_complete(const v34_phase3_stream *s)
{
    return s != NULL && s->cursor.complete;
}

bool v34_phase3_stream_finish_j(v34_phase3_stream *s)
{
    const v34_phase3_event *event;
    if (s == NULL)
        return false;
    event = v34_phase3_current(&s->cursor);
    if (event == NULL || event->signal != V34_P3_J)
        return false;
    if (!v34_phase3_advance(&s->cursor, 0))
        return false;
    enter_event(s);
    return true;
}
