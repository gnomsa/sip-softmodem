#ifndef SIP_SOFTMODEM_JITTER_H
#define SIP_SOFTMODEM_JITTER_H
#include <stddef.h>
#include <stdint.h>
#define JITTER_SLOTS 128
#define JITTER_PAYLOAD 160
struct jitter_slot { uint16_t sequence; uint8_t data[JITTER_PAYLOAD]; int valid; };
struct jitter { struct jitter_slot slots[JITTER_SLOTS]; uint16_t expected; size_t queued; int started, have_expected; };
void jitter_reset(struct jitter *j);
int jitter_put(struct jitter *j,uint16_t sequence,const uint8_t *data,size_t length);
int jitter_get(struct jitter *j,uint8_t *data,size_t capacity);
#endif
