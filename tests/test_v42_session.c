#include "v42_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static size_t take(struct v42_session*s,uint8_t*b){size_t n=v42_session_next(s,b,V42_HDLC_MAX_BITS);assert(n);return n;}
int main(void)
{
    struct v42_xid_params ap={128,96,7,5,V42_XID_SINGLE_SREJ};
    struct v42_xid_params bp={80,112,4,6,V42_XID_SINGLE_SREJ|V42_XID_FCS32};
    struct v42_session a,b;uint8_t bits[V42_HDLC_MAX_BITS],bad[V42_HDLC_MAX_BITS];
    v42_session_init(&a,1,&ap);v42_session_init(&b,0,&bp);
    (void)take(&a,bits);v42_session_advance(&a,1000);size_t n=take(&a,bits);
    assert(a.xid_retries==1&&v42_session_receive(&b,bits,n)==1);n=take(&b,bits);
    assert(v42_session_receive(&a,bits,n)==1&&a.state==V42_SESSION_LINK);
    assert(a.negotiated.n401_tx==112&&a.negotiated.n401_rx==80);
    assert(a.negotiated.k_tx==6&&a.negotiated.k_rx==4);
    n=take(&a,bits);assert(v42_session_receive(&b,bits,n)==1);n=take(&b,bits);
    assert(v42_session_receive(&a,bits,n)==1);
    assert(a.state==V42_SESSION_CONNECTED&&b.state==V42_SESSION_CONNECTED);
    uint8_t source[250],got[250];for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*17u);
    assert(v42_session_write(&a,source,sizeof source)==sizeof source);
    n=take(&a,bits);assert(v42_session_receive(&b,bits,n)==1);
    n=take(&a,bits);memcpy(bad,bits,n);bad[n/2]^=1;assert(v42_session_receive(&b,bad,n)<0);
    n=take(&a,bits);assert(v42_session_receive(&b,bits,n)==0);
    n=take(&b,bits);assert(v42_session_receive(&a,bits,n)==0);
    n=take(&a,bits);assert(v42_session_receive(&b,bits,n)==1);
    n=take(&a,bits);assert(v42_session_receive(&b,bits,n)==1);
    n=take(&b,bits);assert(v42_session_receive(&a,bits,n)==0);
    assert(v42_session_read(&b,got,sizeof got)==sizeof got&&!memcmp(source,got,sizeof got));
    puts("V.42 session: XID retry, negotiation, SABME/UA and REJ recovery pass");return 0;
}
