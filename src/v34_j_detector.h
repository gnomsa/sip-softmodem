#ifndef SOFTMODEM_V34_J_DETECTOR_H
#define SOFTMODEM_V34_J_DETECTOR_H

#include "v34_training_symbols.h"

#include <stdbool.h>
#include <stdint.h>

#define V34_J_CONFIRM_SYMBOLS 16u

typedef struct {
    v34_scrambler initial_scrambler;
    v34_scrambler expected_scrambler;
    unsigned initial_rotation;
    unsigned expected_rotation;
    unsigned bit_index;
    unsigned matched;
    bool detected;
} v34_j_detector;

bool v34_j_detector_init(v34_j_detector *detector,
                         const v34_scrambler *scrambler_after_trn,
                         unsigned final_trn_rotation);
bool v34_j_detector_feed(v34_j_detector *detector, uint8_t phase_pi_6);

#endif
