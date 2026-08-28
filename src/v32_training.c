#include "v32_training.h"
void v32_training_init(struct v32_training*t,enum v32_std_role role){t->segment=V32_SEG_S;t->index=0;v32_std_scrambler_init(&t->scrambler,role);}
static enum v32_carrier_state direct(int dibit){static const enum v32_carrier_state map[4]={V32_STATE_A,V32_STATE_B,V32_STATE_D,V32_STATE_C};return map[dibit&3];}
enum v32_carrier_state v32_training_next(struct v32_training*t){enum v32_carrier_state out;
    if(t->segment==V32_SEG_S){out=(t->index&1)?V32_STATE_B:V32_STATE_A;if(++t->index==256){t->segment=V32_SEG_SBAR;t->index=0;}return out;}
    if(t->segment==V32_SEG_SBAR){out=(t->index&1)?V32_STATE_D:V32_STATE_C;if(++t->index==16){t->segment=V32_SEG_TRN;t->index=0;t->scrambler.history=0;}return out;}
    if(t->segment==V32_SEG_TRN){int a=v32_std_scramble(&t->scrambler,1),b=v32_std_scramble(&t->scrambler,1);out=t->index<256?(a?V32_STATE_C:V32_STATE_A):direct((a<<1)|b);if(++t->index==1280){t->segment=V32_SEG_RATE;t->index=0;}return out;}
    return V32_STATE_A;
}
