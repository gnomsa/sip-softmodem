#include "v34_j_detector.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    v34_scrambler tx;
    v34_j_detector detector;
    unsigned i, bit_index = 0, rotation = 0;
    uint8_t phase;

    v34_scrambler_init(&tx, true);
    for (i = 0; i < 1024; ++i) {
        assert(v34_trn4_phase(&tx, &phase));
        rotation = ((12u - phase) % 12u) / 3u;
    }
    assert(v34_j_detector_init(&detector, &tx, rotation));
    for (i = 0; i < V34_J_CONFIRM_SYMBOLS - 1u; ++i) {
        assert(v34_j4_phase(&tx, &bit_index, &rotation, &phase));
        assert(!v34_j_detector_feed(&detector, phase));
    }
    assert(v34_j4_phase(&tx, &bit_index, &rotation, &phase));
    assert(v34_j_detector_feed(&detector, phase));
    assert(detector.detected);

    assert(v34_j_detector_init(&detector, &detector.initial_scrambler,
                               detector.initial_rotation));
    assert(!v34_j_detector_feed(&detector, 1));
    assert(detector.matched == 0 && !detector.detected);
    puts("v34 symbol-domain J detector tests: ok");
    return 0;
}
