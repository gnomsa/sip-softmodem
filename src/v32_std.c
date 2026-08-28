#include "v32_std.h"
void v32_std_scrambler_init(struct v32_std_scrambler*s,enum v32_std_role role){s->history=0;s->tap=role==V32_STD_CALL?18:5;}
int v32_std_scramble(struct v32_std_scrambler*s,int in){int out=(in&1)^((s->history>>(s->tap-1))&1)^((s->history>>22)&1);s->history=((s->history<<1)|(unsigned)out)&0x7fffff;return out;}
int v32_std_descramble(struct v32_std_scrambler*s,int in){int out=(in&1)^((s->history>>(s->tap-1))&1)^((s->history>>22)&1);s->history=((s->history<<1)|(unsigned)(in&1))&0x7fffff;return out;}
unsigned v32_std_diff_encode(unsigned q,unsigned prev){static const unsigned table[4][4]={{1,3,0,2},{0,1,2,3},{3,2,1,0},{2,0,3,1}};return table[q&3][prev&3];}
unsigned v32bis_trellis_diff_encode(unsigned q,unsigned prev){static const unsigned table[4][4]={{0,1,2,3},{1,0,3,2},{2,3,1,0},{3,2,0,1}};return table[q&3][prev&3];}
int v32bis_trellis_diff_decode(unsigned output,unsigned prev){for(unsigned q=0;q<4;q++)if(v32bis_trellis_diff_encode(q,prev)==(output&3))return(int)q;return-1;}
static uint16_t base(unsigned sync){uint16_t w=(uint16_t)(sync&15);w|=(1u<<7)|(1u<<11)|(1u<<15);return w;}
uint16_t v32_std_rate_word(int r4800,int r9600,int trellis){uint16_t w=base(0);if(r4800)w|=1u<<5;if(r9600)w|=1u<<6;if(trellis)w|=1u<<8;return w;}
uint16_t v32_std_e_word(int rate,int trellis){uint16_t w=base(15);if(rate==4800)w|=1u<<5;if(rate==9600)w|=1u<<6;if(trellis)w|=1u<<8;return w;}
int v32_std_rate_decode(uint16_t w,int*r4800,int*r9600,int*trellis){if((w&0x808f)!=0x8080||(w&(1u<<11))==0)return -1;if(r4800)*r4800=(w>>5)&1;if(r9600)*r9600=(w>>6)&1;if(trellis)*trellis=(w>>8)&1;return ((w>>5)&3)?0:-1;}
int v32_std_e_decode(uint16_t w,int*rate,int*trellis){if((w&0x888f)!=0x888f)return -1;int bits=(w>>5)&3;if(bits!=1&&bits!=2)return -1;if(rate)*rate=bits==2?9600:4800;if(trellis)*trellis=(w>>8)&1;return 0;}
static uint16_t bis_base(unsigned sync){return(uint16_t)(base(sync)|(1u<<4)|(1u<<8));}
uint16_t v32bis_rate_word(unsigned r,int trellis){uint16_t w=bis_base(0);if(r&V32_RATE_4800)w|=1u<<5;if(r&V32_RATE_9600)w|=1u<<6;if(r&V32_RATE_7200)w|=1u<<9;if(r&V32_RATE_12000)w|=1u<<10;if(r&V32_RATE_14400)w|=1u<<12;if(trellis)w|=1u<<8;return w;}
uint16_t v32bis_e_word(int rate,int trellis){unsigned r=rate==4800?V32_RATE_4800:rate==7200?V32_RATE_7200:rate==9600?V32_RATE_9600:rate==12000?V32_RATE_12000:rate==14400?V32_RATE_14400:0;return r?v32bis_rate_word(r,trellis)|15u:0;}
int v32bis_rate_decode(uint16_t w,unsigned*rates,int*trellis,int*bis){if((w&0x888f)!=0x8880)return-1;int is_bis=(w&(1u<<4))&&(w&(1u<<8));unsigned r=0;if(w&(1u<<5))r|=V32_RATE_4800;if(w&(1u<<6))r|=V32_RATE_9600;if(is_bis&&w&(1u<<9))r|=V32_RATE_7200;if(is_bis&&w&(1u<<10))r|=V32_RATE_12000;if(is_bis&&w&(1u<<12))r|=V32_RATE_14400;if(!r)return-1;if(rates)*rates=r;if(trellis)*trellis=0;if(bis)*bis=is_bis;return 0;}
int v32bis_e_decode(uint16_t w,int*rate,int*trellis,int*bis){if((w&15)!=15)return-1;unsigned rates;if(v32bis_rate_decode((uint16_t)(w&~15u),&rates,trellis,bis)<0||!rates||(rates&(rates-1)))return-1;if(rate)*rate=v32_highest_rate(rates);return 0;}
int v32_highest_rate(unsigned r){if(r&V32_RATE_14400)return 14400;if(r&V32_RATE_12000)return 12000;if(r&V32_RATE_9600)return 9600;if(r&V32_RATE_7200)return 7200;if(r&V32_RATE_4800)return 4800;return 0;}
