#ifndef SIP_SOFTMODEM_V42_HDLC_H
#define SIP_SOFTMODEM_V42_HDLC_H
#include <stddef.h>
#include <stdint.h>

#define V42_HDLC_MAX_INFO 256
#define V42_HDLC_MAX_BITS ((V42_HDLC_MAX_INFO+6)*10)

uint16_t v42_hdlc_fcs(const uint8_t *data,size_t count);
size_t v42_hdlc_encode_raw(const uint8_t *frame,size_t frame_count,
                           uint8_t *bits,size_t capacity);
int v42_hdlc_decode_raw(const uint8_t *bits,size_t count,uint8_t *frame,
                        size_t capacity);
size_t v42_hdlc_encode(uint8_t address,uint8_t control,const uint8_t *info,
                       size_t info_count,uint8_t *bits,size_t capacity);
/* Decodes one flag-delimited frame. Returns information length, or -1. */
int v42_hdlc_decode(const uint8_t *bits,size_t count,uint8_t *address,
                    uint8_t *control,uint8_t *info,size_t capacity);
#endif
