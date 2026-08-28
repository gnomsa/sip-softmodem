#include "v22_handshake.h"
#include <string.h>

void v22_handshake_init(struct v22_handshake*h,enum v22_hs_role role){
    memset(h,0,sizeof *h);h->role=role;h->tx=V22_TX_SILENCE;
}
void v22_handshake_answer_sequence_complete(struct v22_handshake*h){
    if(h->role==V22_HS_ANSWER&&h->tx==V22_TX_SILENCE)
        h->tx=V22_TX_UNSCRAMBLED_ONES_1200;
}
static void choose_1200(struct v22_handshake*h){
    h->selected_rate=1200;h->tx=V22_TX_SCRAMBLED_ONES_1200;
    h->deadline_ms=h->now_ms+765;
}
void v22_handshake_event(struct v22_handshake*h,enum v22_hs_event e,unsigned duration){
    if(e==V22_RX_CARRIER_LOST){h->failed=1;h->tx_ready=h->rx_ready=0;return;}
    if(h->role==V22_HS_CALLER&&h->tx==V22_TX_SILENCE&&e==V22_RX_UNSCRAMBLED_ONES&&duration>=145){
        h->deadline_ms=h->now_ms+456;return;
    }
    if(e==V22_RX_RATE_PROBE_0011&&duration>=97){
        h->selected_rate=2400;
        if(h->role==V22_HS_ANSWER){h->tx=V22_TX_RATE_PROBE_0011;h->deadline_ms=h->now_ms+100;}
        else {h->tx=V22_TX_SCRAMBLED_ONES_1200;h->deadline_ms=h->now_ms+600;}
        return;
    }
    if(e==V22_RX_SCRAMBLED_ONES_1200&&duration>=230&&!h->selected_rate){choose_1200(h);h->rx_ready=1;return;}
    if(e==V22_RX_SCRAMBLED_ONES_2400&&duration>=14){h->rx_ready=1;return;} /* 32 bits = 13.3 ms */
}
void v22_handshake_advance(struct v22_handshake*h,unsigned elapsed){
    h->now_ms+=elapsed;if(!h->deadline_ms||h->now_ms<h->deadline_ms)return;
    h->deadline_ms=0;
    if(h->role==V22_HS_CALLER&&h->tx==V22_TX_SILENCE){h->tx=V22_TX_RATE_PROBE_0011;h->deadline_ms=h->now_ms+100;return;}
    if(h->tx==V22_TX_RATE_PROBE_0011){h->tx=V22_TX_SCRAMBLED_ONES_1200;h->deadline_ms=h->now_ms+(h->selected_rate==2400?500:765);return;}
    if(h->selected_rate==2400&&h->tx==V22_TX_SCRAMBLED_ONES_1200){h->tx=V22_TX_SCRAMBLED_ONES_2400;h->deadline_ms=h->now_ms+200;return;}
    if(h->selected_rate==2400&&h->tx==V22_TX_SCRAMBLED_ONES_2400){h->tx=V22_TX_DATA_2400;h->tx_ready=1;return;}
    if(h->selected_rate==1200&&h->tx==V22_TX_SCRAMBLED_ONES_1200){h->tx=V22_TX_DATA_1200;h->tx_ready=1;return;}
}
int v22_handshake_connected(const struct v22_handshake*h){return !h->failed&&h->tx_ready&&h->rx_ready;}
