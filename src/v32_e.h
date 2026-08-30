#ifndef SIP_SOFTMODEM_V32_E_H
#define SIP_SOFTMODEM_V32_E_H
#include "v32_rate.h"

/* One-symbol reconstruction is useful around the expected R3-to-E
 * transition, but becomes a likely false positive if it is left enabled
 * indefinitely while arbitrary user data is examined. */
#define V32_E_MAX_CORRECTION_WORDS 256u

struct v32_e_rx {
    struct v32_std_scrambler descr;
    struct v32_std_scrambler word_start_descr;
    uint16_t word, last;
    unsigned bits, previous, repeats;
    unsigned word_start_previous,words,corrected_symbols;
    unsigned observed[8],observed_count;
    uint16_t expected;
    int expected_ready;
};

void v32_e_rx_init(struct v32_e_rx *r, enum v32_std_role remote_role,
                   enum v32_carrier_state previous);
void v32_e_rx_continue(struct v32_e_rx *r, const struct v32_rate_rx *rate);
void v32_e_rx_expect(struct v32_e_rx *r,uint16_t expected_word);
int v32_e_rx_put(struct v32_e_rx *r, enum v32_carrier_state state,
                 int *rate, int *trellis);
#endif
