#include "v32_std.h"
void v32_std_scrambler_init(struct v32_std_scrambler*s,enum v32_std_role role){s->history=0;s->tap=role==V32_STD_CALL?18:5;}
int v32_std_scramble(struct v32_std_scrambler*s,int in){int out=(in&1)^((s->history>>(s->tap-1))&1)^((s->history>>22)&1);s->history=((s->history<<1)|(unsigned)out)&0x7fffff;return out;}
int v32_std_descramble(struct v32_std_scrambler*s,int in){int out=(in&1)^((s->history>>(s->tap-1))&1)^((s->history>>22)&1);s->history=((s->history<<1)|(unsigned)(in&1))&0x7fffff;return out;}
unsigned v32_std_diff_encode(unsigned q,unsigned prev){static const unsigned table[4][4]={{1,3,0,2},{0,1,2,3},{3,2,1,0},{2,0,3,1}};return table[q&3][prev&3];}
static uint16_t base(unsigned sync){uint16_t w=(uint16_t)(sync&15);w|=(1u<<7)|(1u<<11)|(1u<<15);return w;}
uint16_t v32_std_rate_word(int r4800,int r9600,int trellis){uint16_t w=base(0);if(r4800)w|=1u<<5;if(r9600)w|=1u<<6;if(trellis)w|=1u<<8;return w;}
uint16_t v32_std_e_word(int rate,int trellis){uint16_t w=base(15);if(rate==4800)w|=1u<<5;if(rate==9600)w|=1u<<6;if(trellis)w|=1u<<8;return w;}
int v32_std_rate_decode(uint16_t w,int*r4800,int*r9600,int*trellis){if((w&0x808f)!=0x8080||(w&(1u<<11))==0)return -1;if(r4800)*r4800=(w>>5)&1;if(r9600)*r9600=(w>>6)&1;if(trellis)*trellis=(w>>8)&1;return ((w>>5)&3)?0:-1;}
int v32_std_e_decode(uint16_t w,int*rate,int*trellis){if((w&0x888f)!=0x888f)return -1;int bits=(w>>5)&3;if(bits!=1&&bits!=2)return -1;if(rate)*rate=bits==2?9600:4800;if(trellis)*trellis=(w>>8)&1;return 0;}
