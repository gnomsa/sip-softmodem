#ifndef SOFTMODEM_V34_MP_RECEIVER_H
#define SOFTMODEM_V34_MP_RECEIVER_H
#include "v34_mp.h"
#include "v34_training_symbols.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    v34_scrambler descrambler;
    unsigned rotation;
    unsigned bit_index;
    uint8_t frame[V34_MP0_BYTES];
} v34_mp_receiver;
bool v34_mp_receiver_init(v34_mp_receiver *receiver,
                          const v34_scrambler *descrambler_after_trn,
                          unsigned final_trn_rotation);
bool v34_mp_receiver_feed(v34_mp_receiver *receiver, uint8_t phase_pi_6,
                          v34_mp0 *decoded);
void v34_mp_receiver_next(v34_mp_receiver *receiver);
#endif
