#include "v34_info.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned bits(const uint8_t *p, unsigned first, unsigned n)
{
    unsigned v = 0, i;
    for (i = 0; i < n; ++i)
        v |= ((p[(first + i) / 8u] >> ((first + i) % 8u)) & 1u) << i;
    return v;
}

static void put(uint8_t *p, unsigned first, unsigned n, unsigned v)
{
    unsigned i;
    for (i = 0; i < n; ++i) {
        unsigned o = first + i;
        if (v & (1u << i)) p[o / 8u] |= (uint8_t)(1u << (o % 8u));
        else p[o / 8u] &= (uint8_t)~(1u << (o % 8u));
    }
}

int main(void)
{
    uint8_t c[V34_INFO1C_BYTES] = {0};
    uint8_t a[V34_INFO1A_BYTES] = {0};

    put(c, 0, 4, 0xf); put(c, 4, 8, 0x4e);
    put(c, 12, 77, 0x55aa55aaU);
    put(c, 105, 4, 0xf);
    assert(v34_info_frame_set_crc(c, V34_INFO1C_BITS, 89, 16));
    assert(v34_info_frame_check(c, V34_INFO1C_BITS, 89, 16));
    c[7] ^= 1u;
    assert(!v34_info_frame_check(c, V34_INFO1C_BITS, 89, 16));

    put(a, 0, 4, 0xf); put(a, 4, 8, 0x4e);
    put(a, 12, 38, 0x12345678U);
    put(a, 66, 4, 0xf);
    assert(v34_info_frame_set_crc(a, V34_INFO1A_BITS, 50, 16));
    assert(v34_info_frame_check(a, V34_INFO1A_BITS, 50, 16));
    assert(bits(a, 0, 4) == 0xf && bits(a, 4, 8) == 0x4e);
    a[0] ^= 1u;
    assert(!v34_info_frame_check(a, V34_INFO1A_BITS, 50, 16));
    a[0] ^= 1u;
    put(a, 66, 4, 0xe);
    assert(!v34_info_frame_check(a, V34_INFO1A_BITS, 50, 16));

    {
        const v34_info1a in = {3, 2, 47, true, 8, 14, 5, 4, -137};
        v34_info1a out;
        memset(a, 0, sizeof(a));
        assert(v34_info1a_encode(&in, a));
        assert(v34_info1a_decode(a, &out));
        assert(out.minimum_power_reduction == 3);
        assert(out.additional_power_reduction == 2);
        assert(out.md_length_35ms == 47);
        assert(out.high_carrier && out.preemphasis == 8);
        assert(out.projected_rate_2400 == 14);
        assert(out.answer_symbol_rate == 5 && out.call_symbol_rate == 4);
        assert(out.frequency_offset_002hz == -137);
        a[5] ^= 0x20u;
        assert(!v34_info1a_decode(a, &out));
    }

    {
        v34_info1a invalid = {0};
        invalid.preemphasis = 11;
        assert(!v34_info1a_encode(&invalid, a));
    }
    puts("v34 INFO1 framing tests: ok");
    return 0;
}
