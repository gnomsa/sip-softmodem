#include "v32bis_viterbi.h"
#include "v32bis_trellis.h"
#include "v32bis_map.h"
#include <assert.h>
#include <stdio.h>
static void run(int rate){struct v32bis_trellis enc;struct v32bis_viterbi dec;v32bis_trellis_init(&enc);assert(v32bis_viterbi_init(&dec,rate)==0);int bits=rate/2400,eb=bits-2;uint8_t source[300];unsigned seed=7,received=0;for(unsigned n=0;n<300;n++){seed=seed*1664525u+1013904223u;source[n]=(uint8_t)((seed>>24)&((1u<<bits)-1));unsigned y12=source[n]>>eb,x=source[n]&((1u<<eb)-1),subset=v32bis_trellis_encode(&enc,y12),label=(subset<<eb)|x;struct v32bis_point p;assert(v32bis_map_point(rate,label,&p)==0);double ni=(int)((seed>>8)&7)-3.5,nq=(int)((seed>>12)&7)-3.5;uint8_t got;if(v32bis_viterbi_put(&dec,p.i+ni*.08,p.q+nq*.08,&got)==1)assert(got==source[received++]);}assert(received==300-V32BIS_TRACEBACK+1);}
int main(void){run(7200);run(9600);run(12000);run(14400);assert(v32bis_viterbi_init(0,9600)<0);puts("V.32bis soft Viterbi: noisy 7200/9600/12000/14400 paths pass");return 0;}
