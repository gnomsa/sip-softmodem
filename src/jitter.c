#include "jitter.h"
#include <string.h>

void jitter_reset(struct jitter *j) { memset(j,0,sizeof *j); }
int jitter_put(struct jitter *j,uint16_t seq,const uint8_t *data,size_t n) {
    if(!j||!data||n!=JITTER_PAYLOAD)return -1;
    if(!j->have_expected){j->expected=seq;j->have_expected=1;}
    struct jitter_slot *s=&j->slots[seq%JITTER_SLOTS];
    if(s->valid&&s->sequence==seq)return 0;
    if(s->valid&&j->queued)j->queued--;
    s->sequence=seq;memcpy(s->data,data,n);s->valid=1;j->queued++;
    if(j->queued>=10)j->started=1;
    return 1;
}
int jitter_get(struct jitter *j,uint8_t *data,size_t cap) {
    if(!j||!data||cap<JITTER_PAYLOAD||!j->started)return 0;
    struct jitter_slot *s=&j->slots[j->expected%JITTER_SLOTS];
    if(!s->valid||s->sequence!=j->expected) {
        /* Never synthesize audio: skip a genuinely lost packet after the
           queue has enough future media to prove this is not mere jitter. */
        if(j->queued>20)j->expected++;
        return 0;
    }
    memcpy(data,s->data,JITTER_PAYLOAD);s->valid=0;j->queued--;j->expected++;return JITTER_PAYLOAD;
}
