#include "v34_training_tx.h"
#include "pcma.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void)
{
    v34_training_tx tx;
    long long energy = 0;
    unsigned i;
    assert(v34_training_tx_init(&tx, V34_SYMBOL_3429, true, 8000, 9000));
    for (i = 0; i < 8000; ++i) {
        int sample;
        v34_training_tx_set_phase(&tx, (uint8_t)((i / 3u) % 12u));
        sample = pcma_decode(v34_training_tx_pcma(&tx));
        energy += (long long)sample * sample;
        assert(sample <= 32767 && sample >= -32768);
    }
    assert(energy > 1000000000LL);
    assert(!v34_training_tx_init(&tx, V34_SYMBOL_COUNT, false, 8000, 9000));
    puts("v34 training QAM/PCMA transmitter tests: ok");
    return 0;
}
