#include "v34_mp_receiver.h"
#include <string.h>
bool v34_mp_receiver_init(v34_mp_receiver*r,const v34_scrambler*s,unsigned rotation){if(!r||!s||rotation>3)return false;memset(r,0,sizeof(*r));r->descrambler=*s;r->rotation=rotation;return true;}
void v34_mp_receiver_next(v34_mp_receiver*r){if(!r)return;r->bit_index=0;memset(r->frame,0,sizeof(r->frame));}
bool v34_mp_receiver_feed(v34_mp_receiver*r,uint8_t phase,v34_mp0*decoded){unsigned z,d,k,b[2];if(!r||!decoded||phase>=12||phase%3||r->bit_index>=V34_MP0_BITS)return false;z=((12u-phase)%12u)/3u;d=(z-r->rotation)&3u;r->rotation=z;b[0]=v34_descramble_bit(&r->descrambler,d&1u);b[1]=v34_descramble_bit(&r->descrambler,(d>>1)&1u);for(k=0;k<2;k++,r->bit_index++)if(b[k])r->frame[r->bit_index/8u]|=(uint8_t)(1u<<(r->bit_index%8u));return r->bit_index==V34_MP0_BITS&&v34_mp0_decode(r->frame,decoded);}
