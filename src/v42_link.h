#ifndef SIP_SOFTMODEM_V42_LINK_H
#define SIP_SOFTMODEM_V42_LINK_H
#include "v42_lapm.h"
enum v42_link_state {V42_LINK_DISCONNECTED,V42_LINK_AWAIT_UA,V42_LINK_CONNECTED,V42_LINK_AWAIT_RELEASE,V42_LINK_FAILED};
struct v42_link {enum v42_link_state state;int originator;enum v42_lapm_u pending;int have_pending;uint64_t now_ms,deadline_ms;unsigned t401_ms,retries,max_retries;};
void v42_link_init(struct v42_link*l,int originator);
size_t v42_link_next(struct v42_link*l,uint8_t*bits,size_t capacity);
int v42_link_receive(struct v42_link*l,const uint8_t*bits,size_t count);
void v42_link_advance(struct v42_link*l,unsigned elapsed_ms);
void v42_link_disconnect(struct v42_link*l);
#endif
