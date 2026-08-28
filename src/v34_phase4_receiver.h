#ifndef SOFTMODEM_V34_PHASE4_RECEIVER_H
#define SOFTMODEM_V34_PHASE4_RECEIVER_H

#include "v34_mp_receiver.h"
#include "v34_training_rx.h"

typedef struct {
    v34_training_rx rx;
    v34_scrambler j_scrambler;
    v34_scrambler trn_scrambler;
    v34_scrambler e_scrambler;
    v34_mp_receiver mp_receiver;
    v34_mp0 mp;
    v34_mp0 mp_prime;
    unsigned j_index;
    unsigned j_rotation;
    unsigned trn_rotation;
    unsigned e_index;
    unsigned e_rotation;
    unsigned symbols;
    bool sender_call_modem;
    bool have_mp;
    bool have_mp_prime;
    bool complete;
    bool failed;
} v34_phase4_receiver;

bool v34_phase4_receiver_init(v34_phase4_receiver *receiver,
                              bool sender_call_modem,
                              const v34_scrambler *phase3_scrambler,
                              unsigned phase3_rotation,
                              v34_symbol_rate symbol_rate,
                              bool high_carrier,
                              unsigned sample_rate);
bool v34_phase4_receiver_feed(v34_phase4_receiver *receiver, uint8_t pcma);
bool v34_phase4_receiver_complete(const v34_phase4_receiver *receiver);

#endif
