#include "v42_stream.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    struct v42_stream a,b;v42_stream_init(&a);v42_stream_init(&b);
    uint8_t source[77],frame[V42_HDLC_MAX_BITS],wire[400],got[V42_HDLC_MAX_BITS],raw[100];
    for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*31u+7u);
    size_t nf=v42_hdlc_encode_raw(source,sizeof source,frame,sizeof frame);assert(nf);
    assert(v42_stream_send(&a,frame,nf)==nf);
    size_t nw=v42_stream_tx_bytes(&a,wire,sizeof wire);assert(nw==sizeof wire);
    for(size_t off=0;off<nw;){size_t z=nw-off>7?7:nw-off;v42_stream_rx_bytes(&b,wire+off,z);off+=z;}
    size_t ng=v42_stream_receive(&b,got,sizeof got);assert(ng==nf);
    int nr=v42_hdlc_decode_raw(got,ng,raw,sizeof raw);assert(nr==(int)sizeof source&&!memcmp(raw,source,sizeof source));
    puts("V.42 streaming HDLC: arbitrary byte chunks and idle flags pass");return 0;
}
