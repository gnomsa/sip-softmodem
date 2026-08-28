#ifndef SIP_SOFTMODEM_V22_HANDSHAKE_H
#define SIP_SOFTMODEM_V22_HANDSHAKE_H
#include <stdint.h>

enum v22_hs_role { V22_HS_CALLER, V22_HS_ANSWER };
enum v22_hs_tx {
    V22_TX_SILENCE,
    V22_TX_UNSCRAMBLED_ONES_1200,
    V22_TX_RATE_PROBE_0011,
    V22_TX_SCRAMBLED_ONES_1200,
    V22_TX_SCRAMBLED_ONES_2400,
    V22_TX_DATA_1200,
    V22_TX_DATA_2400
};
enum v22_hs_event {
    V22_RX_NONE,
    V22_RX_UNSCRAMBLED_ONES,
    V22_RX_RATE_PROBE_0011,
    V22_RX_SCRAMBLED_ONES_1200,
    V22_RX_SCRAMBLED_ONES_2400,
    V22_RX_CARRIER_LOST
};
struct v22_handshake {
    enum v22_hs_role role;
    enum v22_hs_tx tx;
    uint64_t now_ms, deadline_ms;
    unsigned stable_ms;
    int selected_rate, tx_ready, rx_ready, failed;
};
void v22_handshake_init(struct v22_handshake *h, enum v22_hs_role role);
void v22_handshake_answer_sequence_complete(struct v22_handshake *h);
void v22_handshake_advance(struct v22_handshake *h, unsigned elapsed_ms);
void v22_handshake_event(struct v22_handshake *h, enum v22_hs_event event,
                         unsigned duration_ms);
int v22_handshake_connected(const struct v22_handshake *h);
#endif
