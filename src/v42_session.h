#ifndef SIP_SOFTMODEM_V42_SESSION_H
#define SIP_SOFTMODEM_V42_SESSION_H
#include "v42_arq.h"
#include "v42_link.h"
#include "v42_xid.h"

enum v42_session_state {
    V42_SESSION_XID,
    V42_SESSION_LINK,
    V42_SESSION_CONNECTED,
    V42_SESSION_FAILED
};

struct v42_session {
    enum v42_session_state state;
    int originator;
    struct v42_xid_params local,negotiated;
    struct v42_link link;
    struct v42_arq arq;
    int xid_pending;
    uint64_t now_ms,xid_deadline_ms;
    unsigned t401_ms,xid_retries,max_retries;
};

void v42_session_init(struct v42_session*s,int originator,
                      const struct v42_xid_params*local);
size_t v42_session_next(struct v42_session*s,uint8_t*bits,size_t capacity);
int v42_session_receive(struct v42_session*s,const uint8_t*bits,size_t count);
void v42_session_advance(struct v42_session*s,unsigned elapsed_ms);
size_t v42_session_write(struct v42_session*s,const uint8_t*data,size_t count);
size_t v42_session_read(struct v42_session*s,uint8_t*data,size_t capacity);
#endif
