#include "v34_mp.h"
#include "v34_caps.h"
#include <assert.h>
#include <stdio.h>
int main(void){v34_mp0 in={14,12,false,2,true,true,false,V34_RATE_ALL_MASK,true},out;uint8_t f[V34_MP0_BYTES];assert(v34_mp0_encode(&in,f));assert(v34_mp0_decode(f,&out));assert(out.call_to_answer_rate_2400==14&&out.answer_to_call_rate_2400==12);assert(out.trellis_encoder==2&&out.nonlinear_encoder&&out.expanded_shaping);assert(out.rate_mask==V34_RATE_ALL_MASK&&out.asymmetric_rates&&!out.acknowledge);in.acknowledge=true;assert(v34_mp0_encode(&in,f));assert(v34_mp0_decode(f,&out)&&out.acknowledge);f[5]^=4;assert(!v34_mp0_decode(f,&out));in.trellis_encoder=3;assert(!v34_mp0_encode(&in,f));puts("v34 MP/MP-prime Type 0 codec tests: ok");return 0;}
