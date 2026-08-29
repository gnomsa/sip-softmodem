#ifndef SIP_SOFTMODEM_V42_V32_H
#define SIP_SOFTMODEM_V42_V32_H
#include "v32_session.h"
#include "v42_session.h"
#include "v42_stream.h"

struct v42_v32 {
    struct v32_session physical;
    struct v42_session lapm;
    struct v42_stream stream;
    uint8_t pending[8192];size_t pending_head,pending_tail;
};
void v42_v32_init(struct v42_v32*m,enum v32_std_role role,int allow_4800,int allow_9600);
void v42_v32_init_rate(struct v42_v32*m,enum v32_std_role role,int max_rate);
void v42_v32_start_standard(struct v42_v32 *m);
void v42_v32_generate(struct v42_v32*m,int16_t*pcm,size_t count);
void v42_v32_receive(struct v42_v32*m,const int16_t*pcm,size_t count);
void v42_v32_media_gap(struct v42_v32*m);
size_t v42_v32_write(struct v42_v32*m,const uint8_t*data,size_t count);
size_t v42_v32_read(struct v42_v32*m,uint8_t*data,size_t capacity);
int v42_v32_connected(const struct v42_v32*m);
int v42_v32_rate(const struct v42_v32*m);
#endif
