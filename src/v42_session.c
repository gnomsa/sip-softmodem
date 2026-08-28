#include "v42_session.h"
#include <string.h>

static unsigned xid_cr(const struct v42_session*s,int response)
{return(unsigned)(response?!s->originator:s->originator);}

static void begin_link(struct v42_session*s)
{
    v42_link_init(&s->link,s->originator);
    v42_arq_init(&s->arq,(unsigned)s->originator);
    v42_arq_configure(&s->arq,s->negotiated.k_tx,s->negotiated.n401_tx);
    s->state=V42_SESSION_LINK;
}

void v42_session_init(struct v42_session*s,int originator,
                      const struct v42_xid_params*local)
{
    memset(s,0,sizeof *s);s->originator=!!originator;s->t401_ms=1000;s->max_retries=3;
    if(local)s->local=*local;else v42_xid_defaults(&s->local);
    s->negotiated=s->local;s->state=V42_SESSION_XID;s->xid_pending=s->originator;
}

size_t v42_session_next(struct v42_session*s,uint8_t*bits,size_t cap)
{
    if(s->state==V42_SESSION_XID&&s->xid_pending){uint8_t info[64];
        size_t z=v42_xid_encode(&s->negotiated,info,sizeof info);if(!z)return 0;
        int response=!s->originator;s->xid_pending=0;
        z=v42_lapm_encode_u(V42_LAPM_XID,xid_cr(s,response),0,info,z,bits,cap);
        if(z&&s->originator)s->xid_deadline_ms=s->now_ms+s->t401_ms;
        if(z&&response)begin_link(s);
        return z;}
    if(s->state==V42_SESSION_LINK){size_t z=v42_link_next(&s->link,bits,cap);
        if(s->link.state==V42_LINK_CONNECTED){s->state=V42_SESSION_CONNECTED;if(!z)z=v42_arq_next(&s->arq,bits,cap);}return z;}
    if(s->state==V42_SESSION_CONNECTED)return v42_arq_next(&s->arq,bits,cap);
    return 0;
}

int v42_session_receive(struct v42_session*s,const uint8_t*bits,size_t n)
{
    struct v42_lapm_frame f;if(v42_lapm_decode(bits,n,&f)<0)return-1;
    if(f.type==V42_LAPM_U&&f.u_control==V42_LAPM_XID){struct v42_xid_params peer;
        if(v42_xid_decode(f.info,f.info_count,&peer)<0)return-1;
        v42_xid_intersect(&s->local,&peer,&s->negotiated);s->xid_deadline_ms=0;s->xid_retries=0;
        if(s->originator)begin_link(s);else{s->state=V42_SESSION_XID;s->xid_pending=1;}return 1;}
    if(s->state==V42_SESSION_LINK){int r=v42_link_receive(&s->link,bits,n);
        if(s->link.state==V42_LINK_CONNECTED&&!s->link.have_pending)s->state=V42_SESSION_CONNECTED;
        if(s->link.state==V42_LINK_FAILED)s->state=V42_SESSION_FAILED;
        return r;}
    if(s->state==V42_SESSION_CONNECTED)return v42_arq_receive(&s->arq,bits,n);
    return 0;
}

void v42_session_advance(struct v42_session*s,unsigned ms)
{
    s->now_ms+=ms;
    if(s->state==V42_SESSION_XID&&s->originator&&s->xid_deadline_ms&&s->now_ms>=s->xid_deadline_ms){
        if(s->xid_retries++>=s->max_retries){s->state=V42_SESSION_FAILED;s->xid_deadline_ms=0;}
        else{s->xid_pending=1;s->xid_deadline_ms=0;}}
    if(s->state==V42_SESSION_LINK){v42_link_advance(&s->link,ms);if(s->link.state==V42_LINK_FAILED)s->state=V42_SESSION_FAILED;}
    else if(s->state==V42_SESSION_CONNECTED){v42_arq_advance(&s->arq,ms);if(s->arq.failed)s->state=V42_SESSION_FAILED;}
}

size_t v42_session_write(struct v42_session*s,const uint8_t*d,size_t n)
{return s->state==V42_SESSION_CONNECTED?v42_arq_write(&s->arq,d,n):0;}
size_t v42_session_read(struct v42_session*s,uint8_t*d,size_t n)
{return s->state==V42_SESSION_CONNECTED?v42_arq_read(&s->arq,d,n):0;}
