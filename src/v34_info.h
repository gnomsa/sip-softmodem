#ifndef SOFTMODEM_V34_INFO_H
#define SOFTMODEM_V34_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V34_INFO0_BITS 49u
#define V34_INFO0_BYTES ((V34_INFO0_BITS + 7u) / 8u)
#define V34_INFO1C_BITS 109u
#define V34_INFO1C_BYTES ((V34_INFO1C_BITS + 7u) / 8u)
#define V34_INFO1A_BITS 70u
#define V34_INFO1A_BYTES ((V34_INFO1A_BITS + 7u) / 8u)

typedef enum {
    V34_CLOCK_INTERNAL = 0,
    V34_CLOCK_RECEIVE = 1,
    V34_CLOCK_EXTERNAL = 2
} v34_clock_source;

typedef struct {
    bool symbol_2743;
    bool symbol_2800;
    bool symbol_3429;
    bool carrier_3000_low;
    bool carrier_3000_high;
    bool carrier_3200_low;
    bool carrier_3200_high;
    bool allow_3429;
    bool power_reduction;
    uint8_t maximum_symbol_rate_difference;
    bool cme;
    bool constellation_1664;
    v34_clock_source clock_source;
    bool acknowledge;
} v34_info0;

uint16_t v34_info_crc(const uint8_t *bits, size_t count);
bool v34_info_frame_check(const uint8_t *frame, size_t bit_count,
                          unsigned crc_first, unsigned crc_bits);
bool v34_info_frame_set_crc(uint8_t *frame, size_t bit_count,
                            unsigned crc_first, unsigned crc_bits);
bool v34_info0_encode(const v34_info0 *info, uint8_t frame[V34_INFO0_BYTES]);
bool v34_info0_decode(const uint8_t frame[V34_INFO0_BYTES], v34_info0 *info);

#endif
