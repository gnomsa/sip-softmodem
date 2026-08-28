#ifndef SIP_SOFTMODEM_V32BIS_VITERBI_H
#define SIP_SOFTMODEM_V32BIS_VITERBI_H
#include <stdint.h>
#define V32BIS_TRACEBACK 32
struct v32bis_viterbi {int rate,info_bits;uint64_t time;double metric[8];uint8_t predecessor[V32BIS_TRACEBACK][8],payload[V32BIS_TRACEBACK][8];};
int v32bis_viterbi_init(struct v32bis_viterbi*v,int rate);
int v32bis_viterbi_put(struct v32bis_viterbi*v,double i,double q,uint8_t*payload);
#endif
