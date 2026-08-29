#include "v42_v32.h"
#include <string.h>

#define PENDING_MASK 8191u
void v42_v32_init(struct v42_v32*m,enum v32_std_role role,int a4,int a9)
{
    memset(m,0,sizeof *m);v32_session_init(&m->physical,role,a4,a9);
    v42_session_init(&m->lapm,role==V32_STD_CALL,0);v42_stream_init(&m->stream);
}
void v42_v32_init_rate(struct v42_v32*m,enum v32_std_role role,int max_rate)
{
    memset(m,0,sizeof *m);if(max_rate>=7200)v32bis_session_init(&m->physical,role,max_rate);else v32_session_init(&m->physical,role,1,0);
    v42_session_init(&m->lapm,role==V32_STD_CALL,0);v42_stream_init(&m->stream);
}
void v42_v32_start_standard(struct v42_v32 *m)
{
    v32_session_start_standard(&m->physical);
}
static void pump_user(struct v42_v32*m)
{
    while(m->pending_head!=m->pending_tail&&m->lapm.state==V42_SESSION_CONNECTED){
        size_t end=m->pending_tail>m->pending_head?m->pending_tail:sizeof m->pending;
        size_t n=v42_session_write(&m->lapm,m->pending+m->pending_head,end-m->pending_head);
        m->pending_head=(m->pending_head+n)&PENDING_MASK;if(!n)break;
    }
}
static void pump_frames(struct v42_v32*m)
{
    uint8_t frame[V42_HDLC_MAX_BITS];
    for(unsigned i=0;i<8;i++){size_t n=v42_session_next(&m->lapm,frame,sizeof frame);if(!n)break;if(!v42_stream_send(&m->stream,frame,n))break;}
}
void v42_v32_generate(struct v42_v32*m,int16_t*pcm,size_t count)
{
    if(v32_session_connected(&m->physical)){v42_session_advance(&m->lapm,(unsigned)(count/8));pump_user(m);pump_frames(m);
        uint8_t wire[32];size_t z=(size_t)v32_session_rate(&m->physical)/500;if(z<8)z=8;if(z>sizeof wire)z=sizeof wire;v42_stream_tx_bytes(&m->stream,wire,z);(void)v32_session_write(&m->physical,wire,z);}
    v32_session_generate(&m->physical,pcm,count);
}
void v42_v32_receive(struct v42_v32*m,const int16_t*pcm,size_t count)
{
    v32_session_receive(&m->physical,pcm,count);uint8_t wire[256],frame[V42_HDLC_MAX_BITS];size_t n;
    while((n=v32_session_read(&m->physical,wire,sizeof wire))!=0)v42_stream_rx_bytes(&m->stream,wire,n);
    while((n=v42_stream_receive(&m->stream,frame,sizeof frame))!=0)(void)v42_session_receive(&m->lapm,frame,n);
    pump_user(m);pump_frames(m);
}
void v42_v32_media_gap(struct v42_v32*m){v32_session_media_gap(&m->physical);}
size_t v42_v32_write(struct v42_v32*m,const uint8_t*d,size_t n)
{
    size_t z=0;while(z<n){size_t next=(m->pending_tail+1)&PENDING_MASK;if(next==m->pending_head)break;m->pending[m->pending_tail]=d[z++];m->pending_tail=next;}pump_user(m);return z;
}
size_t v42_v32_read(struct v42_v32*m,uint8_t*d,size_t n){return v42_session_read(&m->lapm,d,n);}
int v42_v32_connected(const struct v42_v32*m){return m->lapm.state==V42_SESSION_CONNECTED;}
int v42_v32_rate(const struct v42_v32*m){return v32_session_rate(&m->physical);}
