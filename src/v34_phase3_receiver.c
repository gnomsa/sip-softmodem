#include "v34_phase3_receiver.h"

#include <string.h>

static bool expected_phase(v34_phase3_receiver *r,
                           const v34_phase3_event *event, uint8_t *phase)
{
    switch (event->signal) {
    case V34_P3_S:
        return v34_s_phase(r->event_symbol, false, phase);
    case V34_P3_S_BAR:
        return v34_s_phase(r->event_symbol, true, phase);
    case V34_P3_PP:
        return v34_pp_phase(r->event_symbol, phase);
    case V34_P3_TRN:
        if (!v34_trn4_phase(&r->scrambler, phase))
            return false;
        r->trn_rotation = ((12u - *phase) % 12u) / 3u;
        return true;
    default:
        return false;
    }
}

static bool enter_event(v34_phase3_receiver *r)
{
    const v34_phase3_event *event = v34_phase3_current(&r->cursor);
    r->event_samples = 0;
    r->event_symbol = 0;
    if (event == NULL)
        return true;
    if (event->signal == V34_P3_TRN)
        v34_scrambler_init(&r->scrambler,
                           r->sender_role == V34_PHASE3_CALL);
    if (event->signal == V34_P3_J) {
        if (!v34_j_detector_init(&r->j_detector, &r->scrambler,
                                 r->trn_rotation))
            return false;
        r->j_ready = false;
    }
    return true;
}

bool v34_phase3_receiver_init(v34_phase3_receiver *r,
                              v34_phase3_role sender_role,
                              uint8_t md_length_35ms, v34_symbol_rate rate,
                              bool high_carrier, unsigned sample_rate)
{
    if (r == NULL)
        return false;
    memset(r, 0, sizeof(*r));
    r->sender_role = sender_role;
    r->sample_rate = sample_rate;
    if (!v34_phase3_build_tx_plan(sender_role, md_length_35ms, &r->plan) ||
        !v34_phase3_cursor_init(&r->cursor, &r->plan) ||
        !v34_training_rx_init(&r->rx, rate, high_carrier, sample_rate))
        return false;
    v34_scrambler_init(&r->scrambler, sender_role == V34_PHASE3_CALL);
    return enter_event(r);
}

size_t v34_phase3_receiver_feed(v34_phase3_receiver *r,
                                const uint8_t *pcma, size_t count)
{
    size_t i;
    if (r == NULL || pcma == NULL || r->failed || r->cursor.complete)
        return 0;
    for (i = 0; i < count; ++i) {
        const v34_phase3_event *event = v34_phase3_current(&r->cursor);
        uint8_t phase;
        uint8_t expected;
        if (event == NULL)
            break;
        if (event->milliseconds != 0) {
            uint32_t target = event->milliseconds * r->sample_rate / 1000u;
            r->event_samples++;
            if (r->event_samples == target) {
                if (!v34_phase3_advance(&r->cursor, event->milliseconds) ||
                    !enter_event(r)) {
                    r->failed = true;
                    break;
                }
            }
            continue;
        }
        if (!v34_training_rx_pcma(&r->rx, pcma[i], &phase, NULL))
            continue;
        if (event->signal == V34_P3_J) {
            if (v34_j_detector_feed(&r->j_detector, phase))
                r->j_ready = true;
            r->scrambler = r->j_detector.expected_scrambler;
            r->trn_rotation = r->j_detector.expected_rotation;
            continue;
        }
        if (!expected_phase(r, event, &expected) || phase != expected) {
            r->failed = true;
            break;
        }
        if (!v34_phase3_advance(&r->cursor, 1)) {
            r->failed = true;
            break;
        }
        r->event_symbol++;
        if (v34_phase3_current(&r->cursor) != event && !enter_event(r)) {
            r->failed = true;
            break;
        }
    }
    return i;
}

bool v34_phase3_receiver_j_detected(const v34_phase3_receiver *r)
{
    return r != NULL && r->j_ready && !r->failed;
}

bool v34_phase3_receiver_finish_j(v34_phase3_receiver *r)
{
    const v34_phase3_event *event;
    uint8_t phase;
    if (r == NULL || !r->j_ready || r->failed)
        return false;
    event = v34_phase3_current(&r->cursor);
    if (event == NULL || event->signal != V34_P3_J)
        return false;
    /* The transmitter has already prepared the symbol following the final
     * emitted J symbol.  Carry that same scrambler/rotation state into Phase
     * 4 even though the prepared symbol itself is not sent. */
    if (!v34_j4_phase(&r->scrambler, &r->j_detector.bit_index,
                      &r->trn_rotation, &phase) ||
        !v34_phase3_advance(&r->cursor, 0))
        return false;
    return enter_event(r);
}

bool v34_phase3_receiver_complete(const v34_phase3_receiver *r)
{
    return r != NULL && r->cursor.complete && !r->failed;
}

bool v34_phase3_receiver_failed(const v34_phase3_receiver *r)
{
    return r == NULL || r->failed;
}
