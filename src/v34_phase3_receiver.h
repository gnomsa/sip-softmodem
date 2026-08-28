#ifndef SOFTMODEM_V34_PHASE3_RECEIVER_H
#define SOFTMODEM_V34_PHASE3_RECEIVER_H

#include "v34_j_detector.h"
#include "v34_phase3.h"
#include "v34_training_rx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    v34_phase3_plan plan;
    v34_phase3_cursor cursor;
    v34_training_rx rx;
    v34_scrambler scrambler;
    v34_j_detector j_detector;
    v34_phase3_role sender_role;
    unsigned sample_rate;
    uint32_t event_samples;
    uint32_t event_symbol;
    unsigned trn_rotation;
    bool j_ready;
    bool j_mismatch;
    bool failed;
} v34_phase3_receiver;

bool v34_phase3_receiver_init(v34_phase3_receiver *receiver,
                              v34_phase3_role sender_role,
                              uint8_t md_length_35ms, v34_symbol_rate rate,
                              bool high_carrier, unsigned sample_rate);
size_t v34_phase3_receiver_feed(v34_phase3_receiver *receiver,
                                const uint8_t *pcma, size_t count);
bool v34_phase3_receiver_j_detected(const v34_phase3_receiver *receiver);
bool v34_phase3_receiver_j_ended(const v34_phase3_receiver *receiver);
bool v34_phase3_receiver_finish_j(v34_phase3_receiver *receiver);
bool v34_phase3_receiver_complete(const v34_phase3_receiver *receiver);
bool v34_phase3_receiver_failed(const v34_phase3_receiver *receiver);

#endif
