#include "v34_phase2_ranging.h"

#include "pcma.h"

#include <string.h>

#define PHASE2_INITIAL_SILENCE 600u
#define PHASE2_50MS 400u
#define PHASE2_40MS 320u
#define PHASE2_10MS 80u

bool v34_phase2_ranging_init(v34_phase2_ranging *r,
                             v34_info_modem_role role,
                             const v34_info0 *local_info0)
{
    uint8_t frame[V34_INFO0_BYTES];
    v34_info_modem_role remote;
    if (r == NULL || local_info0 == NULL ||
        !v34_info0_encode(local_info0, frame))
        return false;
    memset(r, 0, sizeof(*r));
    r->role = role;
    r->state = V34_PHASE2_RANGING_SILENCE;
    r->local_info0 = *local_info0;
    r->silence_samples = PHASE2_INITIAL_SILENCE;
    remote = role == V34_INFO_CALL_MODEM ? V34_INFO_ANSWER_MODEM :
                                           V34_INFO_CALL_MODEM;
    v34_info_modem_tx_init(&r->info_tx, role);
    v34_info_modem_rx_init(&r->info_rx, remote, V34_INFO0_BITS);
    v34_phase2_tone_tx_init(&r->tone_tx, role);
    v34_phase2_tone_rx_init(&r->tone_rx, remote);
    return v34_info_modem_tx_start(&r->info_tx, frame, V34_INFO0_BITS);
}

static void enter_tone(v34_phase2_ranging *r)
{
    v34_phase2_tone_tx_set_active(&r->tone_tx, true);
    r->state = V34_PHASE2_RANGING_TONE;
    r->tone_samples = 0u;
}

static void generate_one(v34_phase2_ranging *r, uint8_t *sample)
{
    if (r->state == V34_PHASE2_RANGING_SILENCE) {
        *sample = pcma_encode(0);
        if (--r->silence_samples == 0u)
            r->state = V34_PHASE2_RANGING_INFO0;
    } else if (r->state == V34_PHASE2_RANGING_INFO0) {
        v34_info_modem_tx_generate(&r->info_tx, sample, 1u);
        if (v34_info_modem_tx_done(&r->info_tx))
            enter_tone(r);
    } else if (r->state == V34_PHASE2_RANGING_TONE ||
               r->state == V34_PHASE2_RANGING_TONE_REVERSED ||
               (r->role == V34_INFO_ANSWER_MODEM &&
                r->state == V34_PHASE2_RANGING_COMPLETE)) {
        v34_phase2_tone_tx_generate(&r->tone_tx, sample, 1u);
        ++r->tone_samples;

        if (r->reversal_wait != 0u && --r->reversal_wait == 0u) {
            v34_phase2_tone_tx_reverse(&r->tone_tx);
            if (r->role == V34_INFO_CALL_MODEM) {
                r->first_reversal_sent = true;
                r->post_reversal_samples = PHASE2_10MS;
                r->state = V34_PHASE2_RANGING_TONE_REVERSED;
            } else {
                r->second_reversal_sent = true;
                r->state = V34_PHASE2_RANGING_COMPLETE;
            }
        } else if (r->role == V34_INFO_CALL_MODEM &&
                   r->first_reversal_sent &&
                   r->post_reversal_samples != 0u &&
                   --r->post_reversal_samples == 0u) {
            v34_phase2_tone_tx_set_active(&r->tone_tx, false);
        }
    } else {
        *sample = pcma_encode(0);
    }
    ++r->tx_samples;
}

void v34_phase2_ranging_generate(v34_phase2_ranging *r, uint8_t *pcma,
                                 size_t sample_count)
{
    size_t i;
    if (r == NULL || pcma == NULL)
        return;
    for (i = 0; i < sample_count; ++i)
        generate_one(r, &pcma[i]);
}

static void accept_info0(v34_phase2_ranging *r)
{
    uint8_t frame[V34_INFO0_BYTES];
    size_t bits;
    if (!v34_info_modem_rx_read(&r->info_rx, frame, sizeof(frame), &bits) ||
        bits != V34_INFO0_BITS || !v34_info0_decode(frame, &r->peer_info0)) {
        r->state = V34_PHASE2_RANGING_FAILED;
        return;
    }
    r->peer_info_ready = true;
    v34_phase2_tone_rx_init(&r->tone_rx,
        r->role == V34_INFO_CALL_MODEM ? V34_INFO_ANSWER_MODEM :
                                         V34_INFO_CALL_MODEM);
}

static void handle_tone_events(v34_phase2_ranging *r)
{
    unsigned reversals = v34_phase2_tone_rx_reversals(&r->tone_rx);

    if (r->role == V34_INFO_ANSWER_MODEM) {
        if (!r->first_reversal_sent && r->peer_info_ready &&
            r->tone_samples >= PHASE2_50MS &&
            v34_phase2_tone_rx_present(&r->tone_rx)) {
            v34_phase2_tone_tx_reverse(&r->tone_tx);
            r->first_reversal_sent = true;
            r->first_reversal_at = r->tx_samples;
            r->state = V34_PHASE2_RANGING_TONE_REVERSED;
        }
        if (r->first_reversal_sent && !r->second_reversal_sent &&
            reversals > r->observed_reversals) {
            uint64_t elapsed = r->tx_samples - r->first_reversal_at;
            r->round_trip_samples = elapsed > PHASE2_40MS ?
                (unsigned)(elapsed - PHASE2_40MS) : 0u;
            r->reversal_wait = PHASE2_40MS;
        }
    } else {
        if (!r->first_reversal_sent && reversals >= 1u &&
            r->reversal_wait == 0u) {
            r->first_reversal_at = r->tx_samples;
            r->reversal_wait = PHASE2_40MS;
        } else if (r->first_reversal_sent && reversals >= 2u &&
                   r->state != V34_PHASE2_RANGING_COMPLETE) {
            uint64_t elapsed = r->tx_samples - r->first_reversal_at;
            r->round_trip_samples = elapsed > 2u * PHASE2_40MS ?
                (unsigned)(elapsed - 2u * PHASE2_40MS) : 0u;
            r->state = V34_PHASE2_RANGING_COMPLETE;
        }
    }
    r->observed_reversals = reversals;
}

void v34_phase2_ranging_receive(v34_phase2_ranging *r, const uint8_t *pcma,
                                size_t sample_count)
{
    size_t i;
    if (r == NULL || pcma == NULL ||
        r->state == V34_PHASE2_RANGING_FAILED)
        return;
    for (i = 0; i < sample_count; ++i) {
        if (!r->peer_info_ready) {
            v34_info_modem_rx_process(&r->info_rx, &pcma[i], 1u);
            if (v34_info_modem_rx_ready(&r->info_rx))
                accept_info0(r);
        } else {
            v34_phase2_tone_rx_process(&r->tone_rx, &pcma[i], 1u);
        }
    }
    if (r->peer_info_ready)
        handle_tone_events(r);
}

bool v34_phase2_ranging_complete(const v34_phase2_ranging *r)
{
    return r != NULL && r->state == V34_PHASE2_RANGING_COMPLETE;
}

const v34_info0 *v34_phase2_ranging_peer_info(const v34_phase2_ranging *r)
{
    return r != NULL && r->peer_info_ready ? &r->peer_info0 : NULL;
}

unsigned v34_phase2_ranging_round_trip_samples(const v34_phase2_ranging *r)
{
    return r == NULL ? 0u : r->round_trip_samples;
}
