#include "pcma.h"
#include "v32_e.h"
#include "v32_line.h"
#include <assert.h>
#include <stdio.h>

static void check(enum v32_std_role role, int wanted)
{
    struct v32_line tx, rx; struct v32_rate_tx enc; struct v32_e_rx dec;
    v32_line_init(&tx); v32_line_init(&rx);
    v32_rate_tx_init(&enc, role, v32_std_e_word(wanted, 0), V32_STATE_A);
    v32_e_rx_init(&dec, role, V32_STATE_A);
    int found = 0, rate = 0, trellis = -1;
    for (int block = 0; block < 10 && !found; block++) {
        enum v32_carrier_state states[48];
        for (int i = 0; i < 48; i++) states[i] = v32_rate_tx_next(&enc);
        assert(v32_line_write(&tx, states, 48) == 48);
        int16_t x[160], y[160]; uint8_t law[160];
        v32_line_generate(&tx, x, 160); pcma_encode_buffer(x, law, 160);
        pcma_decode_buffer(law, y, 160); v32_line_receive(&rx, y, 160);
        size_t n = v32_line_read(&rx, states, 48);
        for (size_t i = 0; i < n; i++)
            if (v32_e_rx_put(&dec, states[i], &rate, &trellis)) found = 1;
    }
    assert(found && rate == wanted && trellis == 0);
}

int main(void)
{
    check(V32_STD_CALL, 4800); check(V32_STD_CALL, 9600);
    check(V32_STD_ANSWER, 4800); check(V32_STD_ANSWER, 9600);
    puts("V.32 E words: 4800/9600, GPC/GPA through PCMA pass");
    return 0;
}
