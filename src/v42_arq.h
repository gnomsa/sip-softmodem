#ifndef SIP_SOFTMODEM_V42_ARQ_H
#define SIP_SOFTMODEM_V42_ARQ_H
#include "v42_lapm.h"

#define V42_ARQ_WINDOW 8
#define V42_ARQ_CHUNK 128
struct v42_arq_slot {uint8_t data[V42_ARQ_CHUNK];size_t count;int valid;};
struct v42_arq {
    unsigned cr,va,vs,send_cursor,vr;
    struct v42_arq_slot tx[128];
    uint8_t rx[8192];size_t rh,rt;
    enum v42_lapm_type pending_s;int have_pending_s;
    uint64_t now_ms,deadline_ms;unsigned t401_ms,retries,max_retries;int failed;
};
void v42_arq_init(struct v42_arq*a,unsigned cr);
size_t v42_arq_write(struct v42_arq*a,const uint8_t*data,size_t count);
size_t v42_arq_read(struct v42_arq*a,uint8_t*data,size_t capacity);
size_t v42_arq_next(struct v42_arq*a,uint8_t*bits,size_t capacity);
int v42_arq_receive(struct v42_arq*a,const uint8_t*bits,size_t count);
void v42_arq_advance(struct v42_arq*a,unsigned elapsed_ms);
size_t v42_arq_unacked(const struct v42_arq*a);
#endif
