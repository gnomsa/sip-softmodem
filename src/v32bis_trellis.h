#ifndef SIP_SOFTMODEM_V32BIS_TRELLIS_H
#define SIP_SOFTMODEM_V32BIS_TRELLIS_H
#include <stdint.h>
struct v32bis_trellis {uint8_t state;};
void v32bis_trellis_init(struct v32bis_trellis*t);
/* Input and returned subset are Y1:Y2 and Y0:Y1:Y2 respectively. */
uint8_t v32bis_trellis_encode(struct v32bis_trellis*t,unsigned y12);
unsigned v32bis_trellis_next_state(unsigned state,unsigned y12);
uint8_t v32bis_trellis_subset(unsigned state,unsigned y12);
#endif
