#include "v42_hdlc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){static const uint8_t payload[]={0x7e,0xff,0xf8,0x00,'V','4','2'};uint8_t bits[V42_HDLC_MAX_BITS],got[64],a,c;size_t n=v42_hdlc_encode(0x03,0x00,payload,sizeof payload,bits,sizeof bits);assert(n>0);int z=v42_hdlc_decode(bits,n,&a,&c,got,sizeof got);assert(z==(int)sizeof payload&&a==0x03&&c==0&&!memcmp(payload,got,sizeof payload));bits[n/2]^=1;assert(v42_hdlc_decode(bits,n,&a,&c,got,sizeof got)<0);assert(v42_hdlc_fcs((const uint8_t*)"123456789",9)==0x906e);puts("V.42 LAPM HDLC: FCS, flags, bit stuffing and corruption rejection pass");return 0;}
