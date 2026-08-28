#include "v32_e.h"

static unsigned state_pair(enum v32_carrier_state s)
{
    static const unsigned p[4] = {0, 1, 3, 2};
    return p[s & 3];
}

void v32_e_rx_init(struct v32_e_rx *r, enum v32_std_role role,
                   enum v32_carrier_state previous)
{
    *r = (struct v32_e_rx){0};
    v32_std_scrambler_init(&r->descr, role);
    r->previous = state_pair(previous);
}

int v32_e_rx_put(struct v32_e_rx *r, enum v32_carrier_state state,
                 int *rate, int *trellis)
{
    unsigned current = state_pair(state), q;
    for (q = 0; q < 4; q++)
        if (v32_std_diff_encode(q, r->previous) == current) break;
    r->previous = current;
    if (q == 4) return 0;
    int a = v32_std_descramble(&r->descr, (q >> 1) & 1);
    int b = v32_std_descramble(&r->descr, q & 1);
    r->word |= (uint16_t)a << r->bits++;
    r->word |= (uint16_t)b << r->bits++;
    if (r->bits < 16) return 0;
    int decoded_rate, decoded_trellis;
    int valid = v32_std_e_decode(r->word, &decoded_rate, &decoded_trellis) == 0;
    if (valid && r->word == r->last) r->repeats++;
    else r->repeats = valid ? 1 : 0;
    r->last = r->word; r->word = 0; r->bits = 0;
    if (r->repeats < 2) return 0;
    if (rate) *rate = decoded_rate;
    if (trellis) *trellis = decoded_trellis;
    return 1;
}
