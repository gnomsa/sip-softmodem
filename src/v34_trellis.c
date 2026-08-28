#include "v34_trellis.h"

static uint8_t state_bit(uint8_t state, unsigned bit)
{
    return (uint8_t)((state >> bit) & 1u);
}

bool v34_trellis_init(v34_trellis_encoder *encoder, v34_trellis_kind kind)
{
    if (!encoder || kind > V34_TRELLIS_64)
        return false;
    encoder->kind = kind;
    encoder->state = 0;
    return true;
}

uint8_t v34_trellis_output(const v34_trellis_encoder *encoder)
{
    unsigned last;

    if (!encoder || encoder->kind > V34_TRELLIS_64)
        return 0;
    last = 4u + (unsigned)encoder->kind;
    return state_bit(encoder->state, last - 1u);
}

uint8_t v34_trellis_put(v34_trellis_encoder *encoder,
                        uint8_t y4, uint8_t y3, uint8_t y2, uint8_t y1)
{
    uint8_t old;
    uint8_t next = 0;
    uint8_t output;

    if (!encoder || encoder->kind > V34_TRELLIS_64 ||
        y4 > 1u || y3 > 1u || y2 > 1u || y1 > 1u)
        return 0;
    old = encoder->state;
    output = v34_trellis_output(encoder);

    if (encoder->kind == V34_TRELLIS_16) {
        next |= output;
        next |= (uint8_t)((state_bit(old, 0) ^ y2) << 1u);
        next |= (uint8_t)((state_bit(old, 1) ^ y2) << 2u);
        next |= (uint8_t)((state_bit(old, 2) ^ y1) << 3u);
    } else if (encoder->kind == V34_TRELLIS_32) {
        next |= output;
        next |= (uint8_t)((state_bit(old, 0) ^ y2) << 1u);
        next |= (uint8_t)((state_bit(old, 1) ^ y1) << 2u);
        next |= (uint8_t)((state_bit(old, 2) ^ y4) << 3u);
        next |= (uint8_t)((state_bit(old, 3) ^ y2) << 4u);
    } else {
        uint8_t d0 = state_bit(old, 0);
        uint8_t d1 = state_bit(old, 1);
        uint8_t d2 = state_bit(old, 2);
        uint8_t d3 = state_bit(old, 3);
        uint8_t d4 = state_bit(old, 4);
        uint8_t a = d0 ^ d1;
        uint8_t b = d1 ^ y1;

        next |= y4 ^ a ^ (d2 & b);
        next |= (uint8_t)((a ^ d3 ^ y3 ^ (y2 & d2)) << 1u);
        next |= (uint8_t)((b ^ d2) << 2u);
        next |= (uint8_t)(d2 << 3u);
        next |= (uint8_t)(output << 4u);
        next |= (uint8_t)((d4 ^ d2 ^ y2) << 5u);
    }
    encoder->state = next;
    return output;
}

static unsigned modulo8(int value)
{
    int result = value % 8;
    return (unsigned)(result < 0 ? result + 8 : result);
}

bool v34_subset_label(v34_point point, uint8_t *label)
{
    static const uint8_t table[4][4] = {
        {1, 6, 5, 2},
        {4, 3, 0, 7},
        {5, 2, 1, 6},
        {0, 7, 4, 3}
    };
    unsigned column;
    unsigned row;

    if (!label || !(point.re & 1) || !(point.im & 1))
        return false;
    column = modulo8((int)point.re + 3) / 2u;
    row = modulo8(3 - (int)point.im) / 2u;
    *label = table[row][column];
    return true;
}

bool v34_subset_pair_bits(uint8_t first, uint8_t second,
                          uint8_t *y4, uint8_t *y3,
                          uint8_t *y2, uint8_t *y1)
{
    static const uint8_t table[8][8] = {
        {0x0,0x0,0x1,0x1,0x8,0x8,0x9,0x9},
        {0x3,0x2,0x2,0x3,0xb,0xa,0xa,0xb},
        {0x5,0x5,0x4,0x4,0xd,0xd,0xc,0xc},
        {0x6,0x7,0x7,0x6,0xe,0xf,0xf,0xe},
        {0x8,0x8,0x9,0x9,0x0,0x0,0x1,0x1},
        {0xb,0xa,0xa,0xb,0x3,0x2,0x2,0x3},
        {0xd,0xd,0xc,0xc,0x5,0x5,0x4,0x4},
        {0xe,0xf,0xf,0xe,0x6,0x7,0x7,0x6}
    };
    uint8_t value;

    if (first > 7u || second > 7u || !y4 || !y3 || !y2 || !y1)
        return false;
    value = table[first][second];
    *y4 = (uint8_t)((value >> 3u) & 1u);
    *y3 = (uint8_t)((value >> 2u) & 1u);
    *y2 = (uint8_t)((value >> 1u) & 1u);
    *y1 = (uint8_t)(value & 1u);
    return true;
}

uint8_t v34_modulo_bit(v34_point first, v34_point second)
{
    int first_sum = first.re / 2 + first.im / 2;
    int second_sum = second.re / 2 + second.im / 2;
    unsigned first_parity = (unsigned)(first_sum < 0 ? -first_sum : first_sum) & 1u;
    unsigned second_parity =
        (unsigned)(second_sum < 0 ? -second_sum : second_sum) & 1u;
    return (uint8_t)(first_parity ^ second_parity);
}

bool v34_encode_4d_zero_precoder(v34_trellis_encoder *encoder,
                                 v34_point first_quarter,
                                 v34_point second_quarter,
                                 uint8_t z,
                                 uint8_t i1,
                                 uint8_t v0,
                                 v34_point output[2])
{
    uint8_t first_label;
    uint8_t second_label;
    uint8_t y4;
    uint8_t y3;
    uint8_t y2;
    uint8_t y1;
    uint8_t u0;

    if (!encoder || !output || z > 3u || i1 > 1u || v0 > 1u)
        return false;
    output[0] = v34_rotate_clockwise(first_quarter, z);
    u0 = (uint8_t)(v34_trellis_output(encoder) ^ v0);
    output[1] = v34_rotate_clockwise(second_quarter,
                                     z + 2u * i1 + u0);
    if (!v34_subset_label(output[0], &first_label) ||
        !v34_subset_label(output[1], &second_label) ||
        !v34_subset_pair_bits(first_label, second_label,
                              &y4, &y3, &y2, &y1))
        return false;
    (void)v34_trellis_put(encoder, y4, y3, y2, y1);
    return true;
}
