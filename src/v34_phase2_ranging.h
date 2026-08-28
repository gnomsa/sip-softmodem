#ifndef SOFTMODEM_V34_PHASE2_RANGING_H
#define SOFTMODEM_V34_PHASE2_RANGING_H

#include "v34_info.h"
#include "v34_info_modem.h"
#include "v34_phase2_tone.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    V34_PHASE2_RANGING_SILENCE = 0,
    V34_PHASE2_RANGING_INFO0,
    V34_PHASE2_RANGING_TONE,
    V34_PHASE2_RANGING_TONE_REVERSED,
    V34_PHASE2_RANGING_COMPLETE,
    V34_PHASE2_RANGING_FAILED
} v34_phase2_ranging_state;

typedef struct {
    v34_info_modem_role role;
    v34_phase2_ranging_state state;
    v34_info0 local_info0;
    v34_info0 peer_info0;
    v34_info_modem_tx info_tx;
    v34_info_modem_rx info_rx;
    v34_phase2_tone_tx tone_tx;
    v34_phase2_tone_rx tone_rx;
    unsigned silence_samples;
    unsigned tone_samples;
    unsigned reversal_wait;
    unsigned post_reversal_samples;
    unsigned observed_reversals;
    uint64_t tx_samples;
    uint64_t first_reversal_at;
    unsigned round_trip_samples;
    bool peer_info_ready;
    bool first_reversal_sent;
    bool second_reversal_sent;
} v34_phase2_ranging;

bool v34_phase2_ranging_init(v34_phase2_ranging *r,
                             v34_info_modem_role role,
                             const v34_info0 *local_info0);
void v34_phase2_ranging_generate(v34_phase2_ranging *r, uint8_t *pcma,
                                 size_t sample_count);
void v34_phase2_ranging_receive(v34_phase2_ranging *r, const uint8_t *pcma,
                                size_t sample_count);
bool v34_phase2_ranging_complete(const v34_phase2_ranging *r);
const v34_info0 *v34_phase2_ranging_peer_info(const v34_phase2_ranging *r);
unsigned v34_phase2_ranging_round_trip_samples(
    const v34_phase2_ranging *r);

#endif
