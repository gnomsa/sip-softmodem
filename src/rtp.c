#include "rtp.h"
#include <arpa/inet.h>
#include <string.h>

static uint16_t get16(const uint8_t *p) { uint16_t v; memcpy(&v,p,2); return ntohs(v); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return ntohl(v); }
static void put16(uint8_t *p,uint16_t v) { v=htons(v); memcpy(p,&v,2); }
static void put32(uint8_t *p,uint32_t v) { v=htonl(v); memcpy(p,&v,4); }

int rtp_parse(const uint8_t *d, size_t n, struct rtp_packet *o) {
    if (!d || !o || n < 12 || (d[0] >> 6) != 2) return -1;
    size_t off = 12u + (size_t)(d[0] & 15u) * 4u;
    if (off > n) return -1;
    if (d[0] & 0x10) {
        if (off + 4 > n) return -1;
        off += 4u + (size_t)get16(d + off + 2) * 4u;
        if (off > n) return -1;
    }
    size_t end = n;
    if (d[0] & 0x20) { if (!d[n-1] || d[n-1] > end-off) return -1; end -= d[n-1]; }
    o->payload_type=d[1]&0x7f; o->sequence=get16(d+2); o->timestamp=get32(d+4); o->ssrc=get32(d+8);
    o->payload=d+off; o->payload_len=end-off;
    return 0;
}

size_t rtp_build(struct rtp_sender *s,const uint8_t *p,size_t n,uint8_t *o,size_t cap) {
    if (!s || !p || !o || cap < n+12) return 0;
    o[0]=0x80; o[1]=8; put16(o+2,s->sequence++); put32(o+4,s->timestamp); put32(o+8,s->ssrc);
    memcpy(o+12,p,n); s->timestamp += (uint32_t)n; return n+12;
}
