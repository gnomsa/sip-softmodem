#ifndef SIP_SOFTMODEM_V42_XID_H
#define SIP_SOFTMODEM_V42_XID_H
#include <stddef.h>
#include <stdint.h>
#define V42_XID_REQUIRED_FUNCTIONS ((1u<<1)|(1u<<3)|(1u<<7)|(1u<<8)|(1u<<11)|(1u<<15))
#define V42_XID_SINGLE_SREJ (1u<<2)
#define V42_XID_FCS32 (1u<<16)
struct v42_xid_params {unsigned n401_tx,n401_rx,k_tx,k_rx;uint32_t optional_functions;};
void v42_xid_defaults(struct v42_xid_params*p);
size_t v42_xid_encode(const struct v42_xid_params*p,uint8_t*out,size_t capacity);
int v42_xid_decode(const uint8_t*data,size_t count,struct v42_xid_params*p);
/* Result directions are relative to side A. */
void v42_xid_intersect(const struct v42_xid_params*a,const struct v42_xid_params*b,
                       struct v42_xid_params*result);
#endif
