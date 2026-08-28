#include "v34_j_detector.h"

#include <stddef.h>

bool v34_j_detector_init(v34_j_detector *d,
                         const v34_scrambler *scrambler_after_trn,
                         unsigned final_trn_rotation)
{
    if (d == NULL || scrambler_after_trn == NULL || final_trn_rotation > 3u ||
        (scrambler_after_trn->tap != 5u && scrambler_after_trn->tap != 18u))
        return false;
    d->initial_scrambler = *scrambler_after_trn;
    d->expected_scrambler = *scrambler_after_trn;
    d->initial_rotation = final_trn_rotation;
    d->expected_rotation = final_trn_rotation;
    d->bit_index = 0;
    d->matched = 0;
    d->detected = false;
    return true;
}

bool v34_j_detector_feed(v34_j_detector *d, uint8_t phase_pi_6)
{
    uint8_t expected;
    if (d == NULL || phase_pi_6 >= 12u || phase_pi_6 % 3u != 0)
        return false;
    if (!v34_j4_phase(&d->expected_scrambler, &d->bit_index,
                      &d->expected_rotation, &expected))
        return false;
    if (phase_pi_6 == expected) {
        d->matched++;
        if (d->matched >= V34_J_CONFIRM_SYMBOLS)
            d->detected = true;
        return d->detected;
    }

    /* Reacquire at the next candidate symbol from the original TRN state. */
    d->expected_scrambler = d->initial_scrambler;
    d->expected_rotation = d->initial_rotation;
    d->bit_index = 0;
    d->matched = 0;
    d->detected = false;
    return false;
}
