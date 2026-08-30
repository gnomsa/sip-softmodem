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

void v32_e_rx_continue(struct v32_e_rx *r, const struct v32_rate_rx *rate)
{
    *r=(struct v32_e_rx){0};
    r->descr=rate->descr;
    r->previous=rate->previous;
}

void v32_e_rx_expect(struct v32_e_rx *r,uint16_t expected_word)
{
    r->expected=expected_word;r->expected_ready=expected_word!=0;
}

static unsigned expected_symbol_errors(struct v32_e_rx *r,
                                       struct v32_std_scrambler *clean_descr,
                                       unsigned *clean_previous)
{
    struct v32_std_scrambler descr=r->word_start_descr;
    unsigned previous=r->word_start_previous,errors=0;
    for(unsigned symbol=0;symbol<8;symbol++){
        unsigned a=(r->expected>>(symbol*2))&1u;
        unsigned b=(r->expected>>(symbol*2+1))&1u;
        unsigned q=(unsigned)v32_std_scramble(&descr,(int)a)<<1;
        q|=(unsigned)v32_std_scramble(&descr,(int)b);
        unsigned current=v32_std_diff_encode(q,previous);
        if(current!=r->observed[symbol])errors++;
        previous=current;
    }
    if(clean_descr)*clean_descr=descr;
    if(clean_previous)*clean_previous=previous;
    return errors;
}

int v32_e_rx_put(struct v32_e_rx *r, enum v32_carrier_state state,
                 int *rate, int *trellis)
{
    unsigned current = state_pair(state), q;
    if(!r->observed_count){
        r->word_start_descr=r->descr;
        r->word_start_previous=r->previous;
    }
    r->observed[r->observed_count++]=current;
    for (q = 0; q < 4; q++)
        if (v32_std_diff_encode(q, r->previous) == current) break;
    r->previous = current;
    if (q == 4) return 0;
    int a = v32_std_descramble(&r->descr, (q >> 1) & 1);
    int b = v32_std_descramble(&r->descr, q & 1);
    r->word |= (uint16_t)a << r->bits++;
    r->word |= (uint16_t)b << r->bits++;
    if (r->bits < 16) return 0;
    r->words++;r->corrected_symbols=0;
    int decoded_rate, decoded_trellis, bis;
    int valid = v32bis_e_decode(r->word, &decoded_rate, &decoded_trellis, &bis) == 0;
    if (valid && r->word == r->last) r->repeats++;
    else r->repeats = valid ? 1 : 0;
    r->last = r->word; r->word = 0; r->bits = 0;r->observed_count=0;
    /* V.32 5.3.2 defines E as one, and only one, 16-bit sequence. */
    if(!valid&&r->expected_ready){
        struct v32_std_scrambler clean_descr;unsigned clean_previous;
        unsigned errors=expected_symbol_errors(r,&clean_descr,&clean_previous);
        if(errors<=1&&v32bis_e_decode(r->expected,&decoded_rate,
                                     &decoded_trellis,&bis)==0){
            r->descr=clean_descr;r->previous=clean_previous;
            r->corrected_symbols=errors;valid=1;
        }
    }
    if (!valid) return 0;
    if (rate) *rate = decoded_rate;
    if (trellis) *trellis = decoded_trellis;
    return 1;
}
