#ifndef SOFTMODEM_V34_PHASE4_H
#define SOFTMODEM_V34_PHASE4_H
#include "v34_mp.h"
#include "v34_training_symbols.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum {V34_P4_J_PRIME,V34_P4_TRN,V34_P4_MP,V34_P4_MP_PRIME,V34_P4_E,V34_P4_COMPLETE} v34_phase4_state;
typedef struct {
    v34_phase4_state state;
    v34_scrambler scrambler;
    unsigned rotation;
    unsigned index;
    uint8_t mp_frame[V34_MP0_BYTES];
    v34_mp0 mp;
    bool call_modem;
} v34_phase4;
bool v34_phase4_init(v34_phase4 *phase4, bool call_modem,
                     const v34_scrambler *phase3_scrambler,
                     unsigned phase3_rotation, const v34_mp0 *mp);
bool v34_phase4_next(v34_phase4 *phase4, uint8_t *phase_pi_6);
#endif
