#include "v22_handshake.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check failed at line %d: %s\n",__LINE__,#x);return 1;}}while(0)

static int rate_2400(void){
    struct v22_handshake c,a;v22_handshake_init(&c,V22_HS_CALLER);v22_handshake_init(&a,V22_HS_ANSWER);
    v22_handshake_answer_sequence_complete(&a);CHECK(a.tx==V22_TX_UNSCRAMBLED_ONES_1200);
    v22_handshake_event(&c,V22_RX_UNSCRAMBLED_ONES,155);v22_handshake_advance(&c,455);CHECK(c.tx==V22_TX_SILENCE);
    v22_handshake_advance(&c,1);CHECK(c.tx==V22_TX_RATE_PROBE_0011);
    v22_handshake_event(&a,V22_RX_RATE_PROBE_0011,100);CHECK(a.selected_rate==2400&&a.tx==V22_TX_RATE_PROBE_0011);
    v22_handshake_advance(&a,100);v22_handshake_event(&c,V22_RX_RATE_PROBE_0011,100);CHECK(c.selected_rate==2400);
    v22_handshake_advance(&c,600);CHECK(c.tx==V22_TX_SCRAMBLED_ONES_2400);
    v22_handshake_advance(&a,500);CHECK(a.tx==V22_TX_SCRAMBLED_ONES_2400);
    v22_handshake_event(&a,V22_RX_SCRAMBLED_ONES_2400,14);v22_handshake_event(&c,V22_RX_SCRAMBLED_ONES_2400,14);
    v22_handshake_advance(&c,200);v22_handshake_advance(&a,200);
    CHECK(v22_handshake_connected(&c)&&v22_handshake_connected(&a));return 0;
}
static int fallback_1200(void){
    struct v22_handshake c;v22_handshake_init(&c,V22_HS_CALLER);
    v22_handshake_event(&c,V22_RX_UNSCRAMBLED_ONES,155);v22_handshake_advance(&c,456);
    v22_handshake_event(&c,V22_RX_SCRAMBLED_ONES_1200,270);CHECK(c.selected_rate==1200&&c.rx_ready);
    v22_handshake_advance(&c,765);CHECK(v22_handshake_connected(&c));return 0;
}
int main(void){if(rate_2400()||fallback_1200())return 1;puts("V.22/V.22bis handshake tests passed");return 0;}
