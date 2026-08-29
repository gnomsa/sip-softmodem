#include "v32bis_trellis.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    for(unsigned state=0;state<8;state++){unsigned seen=0;
        for(unsigned input=0;input<4;input++){
            /* Figure 1/V.32bis: d1, d2 and d3 are the three delay
             * elements from left to right; Y0 is the current d3. */
            unsigned d1=state&1u,d2=(state>>1)&1u,d3=(state>>2)&1u;
            unsigned y1=(input>>1)&1u,y2=input&1u;
            unsigned next_d1=d3;
            unsigned next_d2=d1^y1^y2^(d3&(d2^y2));
            unsigned next_d3=d2^y2^(y1&d3);
            unsigned expected=next_d1|(next_d2<<1)|(next_d3<<2);
            unsigned ns=v32bis_trellis_next_state(state,input);
            assert(ns==expected);assert(!(seen&(1u<<ns)));seen|=1u<<ns;
            assert(v32bis_trellis_subset(state,input)==((state&4)|input));}}
    struct v32bis_trellis t;v32bis_trellis_init(&t);unsigned inputs[]={0,1,2,3,2,0,1,3};
    for(size_t i=0;i<sizeof inputs/sizeof inputs[0];i++){unsigned state=t.state;
        assert(v32bis_trellis_encode(&t,inputs[i])==((state&4)|inputs[i]));
        unsigned d1=state&1u,d2=(state>>1)&1u,d3=(state>>2)&1u;
        unsigned y1=(inputs[i]>>1)&1u,y2=inputs[i]&1u;
        unsigned expected=d3|((d1^y1^y2^(d3&(d2^y2)))<<1)|
                          ((d2^y2^(y1&d3))<<2);
        assert(t.state==expected);}
    puts("V.32bis 8-state trellis: all 32 transitions and subsets pass");return 0;
}
