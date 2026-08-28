#include "pcma.h"

uint8_t pcma_encode(int16_t input) {
    int sample = input;
    int mask = sample >= 0 ? 0xd5 : 0x55;
    if (sample < 0) sample = -sample - 1;
    sample >>= 3;
    int segment = 0;
    for (int limit = 0x1f; segment < 8 && sample > limit; segment++)
        limit = (limit << 1) | 1;
    if (segment >= 8) return (uint8_t)(0x7f ^ mask);
    int mantissa = (sample >> (segment < 2 ? 1 : segment)) & 0x0f;
    return (uint8_t)(((segment << 4) | mantissa) ^ mask);
}

int16_t pcma_decode(uint8_t value) {
    value ^= 0x55;
    int magnitude = (value & 0x0f) << 4;
    int segment = (value & 0x70) >> 4;
    magnitude += segment == 0 ? 8 : 0x108;
    if (segment > 1) magnitude <<= segment - 1;
    return (int16_t)((value & 0x80) ? magnitude : -magnitude);
}

void pcma_encode_buffer(const int16_t *in, uint8_t *out, size_t count) {
    for (size_t i = 0; i < count; i++) out[i] = pcma_encode(in[i]);
}

void pcma_decode_buffer(const uint8_t *in, int16_t *out, size_t count) {
    for (size_t i = 0; i < count; i++) out[i] = pcma_decode(in[i]);
}
