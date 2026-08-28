#include "v34_mapper.h"

#include <math.h>
#include <string.h>

#define V34_MAX_RINGS 18u
#define V34_MAX_G2_INDEX (2u * (V34_MAX_RINGS - 1u))
#define V34_MAX_G4_INDEX (4u * (V34_MAX_RINGS - 1u))
#define V34_MAX_G8_INDEX (8u * (V34_MAX_RINGS - 1u))

bool v34_mapping_parameters_init(v34_mapping_parameters *p, unsigned b)
{
    unsigned k = 0;
    unsigned q = 0;
    double base;
    unsigned minimum;
    unsigned expanded;

    if (!p || b < 8u || b > 79u)
        return false;
    if (b > 12u) {
        while (b - 12u - 8u * q >= 32u)
            ++q;
        k = b - 12u - 8u * q;
    }
    if (k > V34_MAX_SHELL_BITS || q > V34_MAX_Q_BITS)
        return false;

    base = exp2((double)k / 8.0);
    minimum = (unsigned)ceil(base);
    expanded = (unsigned)floor(1.25 * base + 0.5);
    if (expanded < minimum)
        expanded = minimum;

    p->frame_bits = b;
    p->shell_bits = k;
    p->q_bits = q;
    p->minimum_rings = minimum;
    p->expanded_rings = expanded;
    p->minimum_points = 4u * minimum * (1u << q);
    p->expanded_points = 4u * expanded * (1u << q);
    return true;
}

static bool parse_small(const uint8_t *bits, size_t count,
                        v34_parsed_mapping_frame *out)
{
    unsigned full_groups;
    unsigned j;
    size_t offset = 0;

    if (count == 8u)
        full_groups = 0;
    else if (count == 9u)
        full_groups = 1;
    else if (count == 11u)
        full_groups = 3;
    else if (count == 12u)
        full_groups = 4;
    else
        return false;

    for (j = 0; j < 4u; ++j) {
        out->i[j][0] = bits[offset++];
        out->i[j][1] = bits[offset++];
        if (j < full_groups)
            out->i[j][2] = bits[offset++];
    }
    return offset == count;
}

bool v34_parse_mapping_frame(const v34_mapping_parameters *p,
                             const uint8_t *bits,
                             size_t count,
                             v34_parsed_mapping_frame *out)
{
    size_t offset = 0;
    unsigned j;
    unsigned symbol;
    unsigned bit;

    if (!p || !bits || !out ||
        (count != p->frame_bits && count + 1u != p->frame_bits))
        return false;
    memset(out, 0, sizeof(*out));
    out->low_frame = count + 1u == p->frame_bits;
    out->shell_bit_count = p->shell_bits;
    out->q_bit_count = p->q_bits;

    if (p->frame_bits <= 12u)
        return parse_small(bits, count, out);

    if (out->low_frame) {
        for (bit = 0; bit + 1u < p->shell_bits; ++bit)
            out->shell[bit] = bits[offset++];
        out->shell[p->shell_bits - 1u] = 0;
    } else {
        for (bit = 0; bit < p->shell_bits; ++bit)
            out->shell[bit] = bits[offset++];
    }

    for (j = 0; j < 4u; ++j) {
        for (bit = 0; bit < 3u; ++bit)
            out->i[j][bit] = bits[offset++];
        for (symbol = 0; symbol < 2u; ++symbol)
            for (bit = 0; bit < p->q_bits; ++bit)
                out->q[j][symbol][bit] = bits[offset++];
    }
    return offset == count;
}

static void shell_counts(unsigned rings,
                         uint64_t g2[V34_MAX_G2_INDEX + 1u],
                         uint64_t g4[V34_MAX_G4_INDEX + 1u],
                         uint64_t g8[V34_MAX_G8_INDEX + 1u],
                         uint64_t z8[V34_MAX_G8_INDEX + 2u])
{
    unsigned max2 = 2u * (rings - 1u);
    unsigned max4 = 2u * max2;
    unsigned max8 = 2u * max4;
    unsigned p;
    unsigned n;

    memset(g2, 0, sizeof(uint64_t) * (V34_MAX_G2_INDEX + 1u));
    memset(g4, 0, sizeof(uint64_t) * (V34_MAX_G4_INDEX + 1u));
    memset(g8, 0, sizeof(uint64_t) * (V34_MAX_G8_INDEX + 1u));
    memset(z8, 0, sizeof(uint64_t) * (V34_MAX_G8_INDEX + 2u));

    for (p = 0; p <= max2; ++p) {
        unsigned distance = p > rings - 1u ? p - (rings - 1u) :
                                             (rings - 1u) - p;
        g2[p] = rings - distance;
    }
    for (p = 0; p <= max4; ++p)
        for (n = 0; n <= p; ++n)
            if (n <= max2 && p - n <= max2)
                g4[p] += g2[n] * g2[p - n];
    for (p = 0; p <= max8; ++p) {
        for (n = 0; n <= p; ++n)
            if (n <= max4 && p - n <= max4)
                g8[p] += g4[n] * g4[p - n];
        z8[p + 1u] = z8[p] + g8[p];
    }
}

static unsigned split_rank(uint64_t *rank, unsigned total,
                           const uint64_t *counts, unsigned maximum)
{
    unsigned first;

    for (first = 0; first <= total && first <= maximum; ++first) {
        if (total - first > maximum)
            continue;
        uint64_t combinations = counts[first] * counts[total - first];
        if (*rank < combinations)
            return first;
        *rank -= combinations;
    }
    return maximum + 1u;
}

static void split_pair(unsigned total, unsigned rings, uint64_t rank,
                       uint8_t *first, uint8_t *second)
{
    if (total < rings) {
        *first = (uint8_t)rank;
        *second = (uint8_t)(total - rank);
    } else {
        *second = (uint8_t)(rings - 1u - rank);
        *first = (uint8_t)(total - *second);
    }
}

bool v34_shell_map(const v34_mapping_parameters *p,
                   const uint8_t *bits,
                   bool expanded,
                   uint8_t out[8])
{
    uint64_t g2[V34_MAX_G2_INDEX + 1u];
    uint64_t g4[V34_MAX_G4_INDEX + 1u];
    uint64_t g8[V34_MAX_G8_INDEX + 1u];
    uint64_t z8[V34_MAX_G8_INDEX + 2u];
    uint64_t rank = 0;
    uint64_t r2;
    uint64_t r3;
    uint64_t r4;
    uint64_t r5;
    unsigned rings;
    unsigned max8;
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned bit;

    if (!p || !out || (p->shell_bits && !bits))
        return false;
    rings = expanded ? p->expanded_rings : p->minimum_rings;
    if (p->shell_bits > V34_MAX_SHELL_BITS || rings == 0u ||
        rings > V34_MAX_RINGS)
        return false;
    for (bit = 0; bit < p->shell_bits; ++bit) {
        if (bits[bit] > 1u)
            return false;
        rank |= (uint64_t)bits[bit] << bit;
    }

    shell_counts(rings, g2, g4, g8, z8);
    max8 = 8u * (rings - 1u);
    for (a = 0; a <= max8 && z8[a + 1u] <= rank; ++a)
        ;
    if (a > max8 || rank < z8[a])
        return false;
    rank -= z8[a];

    b = split_rank(&rank, a, g4, V34_MAX_G4_INDEX);
    if (b > V34_MAX_G4_INDEX || g4[b] == 0u)
        return false;
    r2 = rank % g4[b];
    r3 = rank / g4[b];

    c = split_rank(&r2, b, g2, V34_MAX_G2_INDEX);
    d = split_rank(&r3, a - b, g2, V34_MAX_G2_INDEX);
    if (c > V34_MAX_G2_INDEX || d > V34_MAX_G2_INDEX ||
        g2[c] == 0u || g2[d] == 0u)
        return false;
    r4 = r2 % g2[c];
    r5 = r3 % g2[d];
    split_pair(c, rings, r4, &out[0], &out[1]);
    split_pair(b - c, rings, r2 / g2[c], &out[2], &out[3]);
    split_pair(d, rings, r5, &out[4], &out[5]);
    split_pair(a - b - d, rings, r3 / g2[d], &out[6], &out[7]);
    return true;
}

static uint64_t convolution_prefix(const uint64_t *counts,
                                   unsigned maximum,
                                   unsigned total,
                                   unsigned first)
{
    uint64_t rank = 0;
    unsigned index;

    for (index = 0; index < first; ++index)
        if (index <= maximum && total >= index &&
            total - index <= maximum)
            rank += counts[index] * counts[total - index];
    return rank;
}

bool v34_shell_unmap(const v34_mapping_parameters *p,
                     const uint8_t rings_in[8],
                     bool expanded,
                     uint8_t *bits)
{
    uint64_t g2[V34_MAX_G2_INDEX + 1u];
    uint64_t g4[V34_MAX_G4_INDEX + 1u];
    uint64_t g8[V34_MAX_G8_INDEX + 1u];
    uint64_t z8[V34_MAX_G8_INDEX + 2u];
    uint64_t r4;
    uint64_t r5;
    uint64_t r2;
    uint64_t r3;
    uint64_t rank;
    uint64_t limit;
    unsigned rings;
    unsigned a = 0;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned e;
    unsigned f;
    unsigned g;
    unsigned h;
    unsigned index;

    if (!p || !rings_in || (p->shell_bits && !bits))
        return false;
    rings = expanded ? p->expanded_rings : p->minimum_rings;
    if (rings == 0u || rings > V34_MAX_RINGS ||
        p->shell_bits > V34_MAX_SHELL_BITS)
        return false;
    for (index = 0; index < 8u; ++index) {
        if (rings_in[index] >= rings)
            return false;
        a += rings_in[index];
    }
    b = rings_in[0] + rings_in[1] + rings_in[2] + rings_in[3];
    c = rings_in[0] + rings_in[1];
    d = rings_in[4] + rings_in[5];
    e = c < rings ? rings_in[0] : rings - 1u - rings_in[1];
    f = b - c < rings ? rings_in[2] : rings - 1u - rings_in[3];
    g = d < rings ? rings_in[4] : rings - 1u - rings_in[5];
    h = a - b - d < rings ? rings_in[6] : rings - 1u - rings_in[7];

    shell_counts(rings, g2, g4, g8, z8);
    if (c > V34_MAX_G2_INDEX || d > V34_MAX_G2_INDEX ||
        b > V34_MAX_G4_INDEX || a > V34_MAX_G8_INDEX)
        return false;
    r4 = (uint64_t)f * g2[c] + e;
    r5 = (uint64_t)h * g2[d] + g;
    r2 = convolution_prefix(g2, V34_MAX_G2_INDEX, b, c) + r4;
    r3 = convolution_prefix(g2, V34_MAX_G2_INDEX, a - b, d) + r5;
    rank = z8[a] +
           convolution_prefix(g4, V34_MAX_G4_INDEX, a, b) +
           r3 * g4[b] + r2;
    limit = UINT64_C(1) << p->shell_bits;
    if (rank >= limit)
        return false;
    for (index = 0; index < p->shell_bits; ++index)
        bits[index] = (uint8_t)((rank >> index) & 1u);
    return true;
}
