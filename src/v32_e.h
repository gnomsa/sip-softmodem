#ifndef SIP_SOFTMODEM_V32_E_H
#define SIP_SOFTMODEM_V32_E_H
#include "v32_rate.h"

struct v32_e_rx {
    struct v32_std_scrambler descr;
    uint16_t word, last;
    unsigned bits, previous, repeats;
};

void v32_e_rx_init(struct v32_e_rx *r, enum v32_std_role remote_role,
                   enum v32_carrier_state previous);
int v32_e_rx_put(struct v32_e_rx *r, enum v32_carrier_state state,
                 int *rate, int *trellis);
#endif
