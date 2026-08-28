#ifndef SIP_SOFTMODEM_PCMA_H
#define SIP_SOFTMODEM_PCMA_H
#include <stddef.h>
#include <stdint.h>
uint8_t pcma_encode(int16_t sample);
int16_t pcma_decode(uint8_t value);
void pcma_encode_buffer(const int16_t *in, uint8_t *out, size_t count);
void pcma_decode_buffer(const uint8_t *in, int16_t *out, size_t count);
#endif
