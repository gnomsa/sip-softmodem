#ifndef SOFTMODEM_V34_PHASE2_TONE_H
#define SOFTMODEM_V34_PHASE2_TONE_H

#include "v34_info_modem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    v34_info_modem_role role;
    double carrier_phase;
    double guard_phase;
    int polarity;
    bool active;
} v34_phase2_tone_tx;

typedef struct {
    v34_info_modem_role remote_role;
    double carrier_phase;
    double i_acc;
    double q_acc;
    double previous_i;
    double previous_q;
    size_t block_samples;
    unsigned strong_blocks;
    unsigned weak_blocks;
    unsigned reversals;
    bool previous_strong;
    bool present;
} v34_phase2_tone_rx;

void v34_phase2_tone_tx_init(v34_phase2_tone_tx *tx,
                             v34_info_modem_role role);
void v34_phase2_tone_tx_set_active(v34_phase2_tone_tx *tx, bool active);
void v34_phase2_tone_tx_reverse(v34_phase2_tone_tx *tx);
void v34_phase2_tone_tx_generate(v34_phase2_tone_tx *tx, uint8_t *pcma,
                                 size_t sample_count);

void v34_phase2_tone_rx_init(v34_phase2_tone_rx *rx,
                             v34_info_modem_role remote_role);
void v34_phase2_tone_rx_process(v34_phase2_tone_rx *rx, const uint8_t *pcma,
                                size_t sample_count);
bool v34_phase2_tone_rx_present(const v34_phase2_tone_rx *rx);
unsigned v34_phase2_tone_rx_reversals(const v34_phase2_tone_rx *rx);

#endif
