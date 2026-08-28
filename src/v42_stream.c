#include "v42_stream.h"
#include <string.h>

#define BIT_MASK (V42_STREAM_BITS-1u)
static size_t tx_free(const struct v42_stream*s)
{return(s->tx_head-s->tx_tail-1)&BIT_MASK;}
static int is_flag(const uint8_t*b,size_t n)
{if(n<8)return 0;unsigned v=0;for(unsigned i=0;i<8;i++)v|=(unsigned)b[n-8+i]<<i;return v==0x7e;}
void v42_stream_init(struct v42_stream*s){memset(s,0,sizeof *s);}
size_t v42_stream_send(struct v42_stream*s,const uint8_t*b,size_t n)
{
    if(!b||n>tx_free(s))return 0;
    for(size_t i=0;i<n;i++){s->tx[s->tx_tail]=b[i]&1u;s->tx_tail=(s->tx_tail+1)&BIT_MASK;}
    return n;
}
size_t v42_stream_tx_bytes(struct v42_stream*s,uint8_t*out,size_t cap)
{
    size_t z=0;while(z<cap){unsigned byte=0;
        for(unsigned b=0;b<8;b++){uint8_t bit;
            if(s->tx_head!=s->tx_tail){bit=s->tx[s->tx_head];s->tx_head=(s->tx_head+1)&BIT_MASK;}
            else bit=(uint8_t)((0x7e>>b)&1);
            byte|=(unsigned)bit<<b;}
        out[z++]=(uint8_t)byte;
    }return z;
}
static void queue_frame(struct v42_stream*s)
{
    size_t next=(s->frame_tail+1)%V42_STREAM_FRAMES;
    if(next==s->frame_head||s->assembling_count>V42_HDLC_MAX_BITS)return;
    struct v42_stream_frame*f=&s->frames[s->frame_tail];
    memcpy(f->bits,s->assembling,s->assembling_count);f->count=s->assembling_count;s->frame_tail=next;
}
size_t v42_stream_rx_bytes(struct v42_stream*s,const uint8_t*bytes,size_t n)
{
    size_t queued=0;for(size_t j=0;j<n;j++)for(unsigned b=0;b<8;b++){
        uint8_t bit=(bytes[j]>>b)&1u;
        if(s->assembling_count==sizeof s->assembling){s->assembling_count=0;s->in_frame=0;}
        s->assembling[s->assembling_count++]=bit;
        if(is_flag(s->assembling,s->assembling_count)){
            if(s->in_frame&&s->assembling_count>16){queue_frame(s);queued++;}
            memmove(s->assembling,s->assembling+s->assembling_count-8,8);
            s->assembling_count=8;s->in_frame=1;
        }
    }return queued;
}
size_t v42_stream_receive(struct v42_stream*s,uint8_t*bits,size_t cap)
{
    if(s->frame_head==s->frame_tail)return 0;
    struct v42_stream_frame*f=&s->frames[s->frame_head];
    if(f->count>cap)return 0;
    memcpy(bits,f->bits,f->count);size_t n=f->count;
    s->frame_head=(s->frame_head+1)%V42_STREAM_FRAMES;return n;
}
