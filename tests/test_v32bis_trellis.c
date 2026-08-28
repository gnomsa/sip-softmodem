#include "v32bis_trellis.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    static const unsigned expected[8][4]={
        {0,6,2,4},{2,4,0,6},{4,2,6,0},{6,0,4,2},
        {1,5,7,3},{3,7,5,1},{7,3,1,5},{5,1,3,7}};
    for(unsigned state=0;state<8;state++){unsigned seen=0;
        for(unsigned input=0;input<4;input++){unsigned ns=v32bis_trellis_next_state(state,input);
            assert(ns==expected[state][input]);assert(!(seen&(1u<<ns)));seen|=1u<<ns;
            assert(v32bis_trellis_subset(state,input)==((state&4)|input));}}
    struct v32bis_trellis t;v32bis_trellis_init(&t);unsigned inputs[]={0,1,2,3,2,0,1,3};
    for(size_t i=0;i<sizeof inputs/sizeof inputs[0];i++){unsigned state=t.state;
        assert(v32bis_trellis_encode(&t,inputs[i])==((state&4)|inputs[i]));
        assert(t.state==expected[state][inputs[i]]);}
    puts("V.32bis 8-state trellis: all 32 transitions and subsets pass");return 0;
}
