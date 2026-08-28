#include "v34_phase4.h"
#include "v34_caps.h"
#include <assert.h>
#include <stdio.h>
int main(void){v34_scrambler s;v34_phase4 p;v34_mp0 mp={14,14,false,0,false,false,false,V34_RATE_ALL_MASK,true};uint8_t phase;unsigned n=0;v34_scrambler_init(&s,true);assert(v34_phase4_init(&p,true,&s,0,&mp));while(v34_phase4_next(&p,&phase)){assert(phase<12&&phase%3==0);n++;}assert(p.state==V34_P4_COMPLETE);assert(n==8+512+44+44+10);assert(p.mp.acknowledge);puts("v34 Phase 4 J-prime/TRN/MP/MP-prime/E plan tests: ok");return 0;}
