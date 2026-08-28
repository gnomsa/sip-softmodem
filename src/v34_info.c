#include "v34_info.h"

#include <string.h>

static unsigned get_bits(const uint8_t *frame, unsigned first, unsigned width)
{
    unsigned value = 0;
    unsigned i;

    for (i = 0; i < width; ++i)
        value |= ((frame[(first + i) / 8u] >> ((first + i) % 8u)) & 1u) << i;
    return value;
}

static void put_bits(uint8_t *frame, unsigned first, unsigned width, unsigned value)
{
    unsigned i;

    for (i = 0; i < width; ++i) {
        unsigned offset = first + i;
        uint8_t mask = (uint8_t)(1u << (offset % 8u));
        if (value & (1u << i))
            frame[offset / 8u] |= mask;
        else
            frame[offset / 8u] &= (uint8_t)~mask;
    }
}

uint16_t v34_info_crc(const uint8_t *bits, size_t count)
{
    uint16_t crc = 0xffffu;
    size_t i;

    if (bits == NULL && count != 0)
        return 0;
    for (i = 0; i < count; ++i) {
        unsigned input = (bits[i / 8u] >> (i % 8u)) & 1u;
        unsigned feedback = (crc ^ input) & 1u;
        crc >>= 1;
        if (feedback)
            crc ^= 0x8408u;
    }
    return crc;
}

static bool info0_fields_valid(const v34_info0 *info)
{
    return info != NULL && info->maximum_symbol_rate_difference <= 5u &&
           (unsigned)info->clock_source <= V34_CLOCK_EXTERNAL;
}

bool v34_info0_encode(const v34_info0 *info, uint8_t frame[V34_INFO0_BYTES])
{
    uint8_t crc_input[3] = {0};
    uint16_t crc;

    if (!info0_fields_valid(info) || frame == NULL)
        return false;
    memset(frame, 0, V34_INFO0_BYTES);
    put_bits(frame, 0, 4, 0x0fu);
    /* 01110010 in time order; frame storage is also transmitted LSB first. */
    put_bits(frame, 4, 8, 0x4eu);
    put_bits(frame, 12, 1, info->symbol_2743);
    put_bits(frame, 13, 1, info->symbol_2800);
    put_bits(frame, 14, 1, info->symbol_3429);
    put_bits(frame, 15, 1, info->carrier_3000_low);
    put_bits(frame, 16, 1, info->carrier_3000_high);
    put_bits(frame, 17, 1, info->carrier_3200_low);
    put_bits(frame, 18, 1, info->carrier_3200_high);
    put_bits(frame, 19, 1, info->allow_3429);
    put_bits(frame, 20, 1, info->power_reduction);
    put_bits(frame, 21, 3, info->maximum_symbol_rate_difference);
    put_bits(frame, 24, 1, info->cme);
    put_bits(frame, 25, 1, info->constellation_1664);
    put_bits(frame, 26, 2, info->clock_source);
    put_bits(frame, 28, 1, info->acknowledge);

    /* CRC covers INFO0 bits 12..28, in transmission order. */
    put_bits(crc_input, 0, 17, get_bits(frame, 12, 17));
    crc = v34_info_crc(crc_input, 17);
    put_bits(frame, 29, 16, crc);
    put_bits(frame, 45, 4, 0x0fu);
    return true;
}

bool v34_info0_decode(const uint8_t frame[V34_INFO0_BYTES], v34_info0 *info)
{
    uint8_t crc_input[3] = {0};
    uint16_t expected;

    if (frame == NULL || info == NULL || get_bits(frame, 0, 4) != 0x0fu ||
        get_bits(frame, 4, 8) != 0x4eu || get_bits(frame, 45, 4) != 0x0fu)
        return false;
    put_bits(crc_input, 0, 17, get_bits(frame, 12, 17));
    expected = v34_info_crc(crc_input, 17);
    if (get_bits(frame, 29, 16) != expected)
        return false;

    memset(info, 0, sizeof(*info));
    info->symbol_2743 = get_bits(frame, 12, 1) != 0;
    info->symbol_2800 = get_bits(frame, 13, 1) != 0;
    info->symbol_3429 = get_bits(frame, 14, 1) != 0;
    info->carrier_3000_low = get_bits(frame, 15, 1) != 0;
    info->carrier_3000_high = get_bits(frame, 16, 1) != 0;
    info->carrier_3200_low = get_bits(frame, 17, 1) != 0;
    info->carrier_3200_high = get_bits(frame, 18, 1) != 0;
    info->allow_3429 = get_bits(frame, 19, 1) != 0;
    info->power_reduction = get_bits(frame, 20, 1) != 0;
    info->maximum_symbol_rate_difference = (uint8_t)get_bits(frame, 21, 3);
    info->cme = get_bits(frame, 24, 1) != 0;
    info->constellation_1664 = get_bits(frame, 25, 1) != 0;
    info->clock_source = (v34_clock_source)get_bits(frame, 26, 2);
    info->acknowledge = get_bits(frame, 28, 1) != 0;
    return info0_fields_valid(info);
}
