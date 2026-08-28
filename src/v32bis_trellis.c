#include "v32bis_trellis.h"
static const uint8_t next[8][4]={
    {0,6,2,4},{2,4,0,6},{4,2,6,0},{6,0,4,2},
    {1,5,7,3},{3,7,5,1},{7,3,1,5},{5,1,3,7}
};
void v32bis_trellis_init(struct v32bis_trellis*t){t->state=0;}
unsigned v32bis_trellis_next_state(unsigned state,unsigned y12){return next[state&7][y12&3];}
uint8_t v32bis_trellis_subset(unsigned state,unsigned y12){return(uint8_t)((state&4)|(y12&3));}
uint8_t v32bis_trellis_encode(struct v32bis_trellis*t,unsigned y12){uint8_t out=v32bis_trellis_subset(t->state,y12);t->state=(uint8_t)v32bis_trellis_next_state(t->state,y12);return out;}
