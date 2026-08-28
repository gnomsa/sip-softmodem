#ifndef SIP_SOFTMODEM_V42_LAPM_H
#define SIP_SOFTMODEM_V42_LAPM_H
#include "v42_hdlc.h"

enum v42_lapm_type { V42_LAPM_I,V42_LAPM_RR,V42_LAPM_RNR,V42_LAPM_REJ,V42_LAPM_SREJ,V42_LAPM_U };
struct v42_lapm_frame {enum v42_lapm_type type;unsigned dlci,cr,ns,nr,pf;uint8_t u_control;uint8_t info[V42_HDLC_MAX_INFO];size_t info_count;};
size_t v42_lapm_encode_i(unsigned cr,unsigned ns,unsigned nr,unsigned poll,
                         const uint8_t*info,size_t count,uint8_t*bits,size_t capacity);
size_t v42_lapm_encode_s(enum v42_lapm_type type,unsigned cr,unsigned nr,
                         unsigned pf,uint8_t*bits,size_t capacity);
int v42_lapm_decode(const uint8_t*bits,size_t count,struct v42_lapm_frame*frame);
#endif
