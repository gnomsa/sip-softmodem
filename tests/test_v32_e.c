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

static void check_rate_to_e_continuity(enum v32_std_role role, int wanted)
{
    struct v32_rate_tx rate_tx, e_tx;
    struct v32_rate_rx rate_rx;
    struct v32_e_rx e_rx;
    uint16_t rate_word=v32_std_rate_word(1,wanted==9600,0), got=0;
    enum v32_carrier_state state=V32_STATE_A;
    v32_rate_tx_init(&rate_tx,role,rate_word,state);
    v32_rate_rx_init(&rate_rx,role,state);
    for(unsigned n=0;n<16;n++){
        state=v32_rate_tx_next(&rate_tx);
        (void)v32_rate_rx_put(&rate_rx,state,&got);
    }
    assert(rate_rx.detected&&got==rate_word);
    v32_e_rx_continue(&e_rx,&rate_rx);
    v32_rate_tx_continue(&e_tx,&rate_tx,v32_std_e_word(wanted,0));
    int found=0, decoded_rate=0, trellis=-1;
    for(unsigned n=0;n<8;n++){
        state=v32_rate_tx_next(&e_tx);
        if(v32_e_rx_put(&e_rx,state,&decoded_rate,&trellis))found=1;
    }
    assert(found&&decoded_rate==wanted&&trellis==0);
}

static void check_single_symbol_recovery(enum v32_std_role role)
{
    struct v32_rate_tx rate_tx,e_tx;
    struct v32_rate_rx rate_rx;
    struct v32_e_rx e_rx;
    uint16_t rate_word=v32bis_rate_word(V32_RATE_9600,1),got=0;
    uint16_t e_word=v32bis_e_word(9600,1);
    enum v32_carrier_state state=V32_STATE_A,e_states[8];
    v32_rate_tx_init(&rate_tx,role,rate_word,state);
    v32_rate_rx_init(&rate_rx,role,state);
    for(unsigned n=0;n<32;n++){
        state=v32_rate_tx_next(&rate_tx);
        (void)v32_rate_rx_put(&rate_rx,state,&got);
    }
    assert(rate_rx.detected&&got==rate_word);
    v32_e_rx_continue(&e_rx,&rate_rx);
    v32_e_rx_expect(&e_rx,e_word);
    v32_rate_tx_continue(&e_tx,&rate_tx,e_word);
    for(unsigned n=0;n<8;n++)e_states[n]=v32_rate_tx_next(&e_tx);
    e_states[3]=(enum v32_carrier_state)((e_states[3]+1)&3);
    int found=0,rate=0,trellis=0;
    for(unsigned n=0;n<8;n++)
        if(v32_e_rx_put(&e_rx,e_states[n],&rate,&trellis))found=1;
    assert(found&&rate==9600&&trellis==1&&e_rx.corrected_symbols==1);

    v32_e_rx_continue(&e_rx,&rate_rx);
    v32_e_rx_expect(&e_rx,e_word);
    e_states[3]=(enum v32_carrier_state)((e_states[3]+3)&3);
    e_states[3]=(enum v32_carrier_state)((e_states[3]+1)&3);
    e_states[5]=(enum v32_carrier_state)((e_states[5]+1)&3);
    found=0;
    for(unsigned n=0;n<8;n++)
        if(v32_e_rx_put(&e_rx,e_states[n],&rate,&trellis))found=1;
    assert(!found);
}

int main(void)
{
    check(V32_STD_CALL, 4800); check(V32_STD_CALL, 9600);
    check(V32_STD_ANSWER, 4800); check(V32_STD_ANSWER, 9600);
    check_rate_to_e_continuity(V32_STD_CALL,9600);
    check_rate_to_e_continuity(V32_STD_ANSWER,9600);
    check_single_symbol_recovery(V32_STD_CALL);
    check_single_symbol_recovery(V32_STD_ANSWER);
    puts("V.32 E words: 4800/9600, GPC/GPA through PCMA pass");
    return 0;
}
