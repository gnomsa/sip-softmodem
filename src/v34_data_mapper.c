#include "v34_data_mapper.h"

#include <stdlib.h>
#include <string.h>

#define V34_LATTICE_SIDE 33u
#define V34_LATTICE_POINTS (V34_LATTICE_SIDE * V34_LATTICE_SIDE)

typedef struct {
    v34_point point;
    unsigned energy;
} ranked_point;

static int compare_points(const void *left, const void *right)
{
    const ranked_point *a = left;
    const ranked_point *b = right;

    if (a->energy != b->energy)
        return a->energy < b->energy ? -1 : 1;
    if (a->point.im != b->point.im)
        return a->point.im > b->point.im ? -1 : 1;
    if (a->point.re != b->point.re)
        return a->point.re > b->point.re ? -1 : 1;
    return 0;
}

void v34_constellation_init(v34_constellation *constellation)
{
    ranked_point candidates[V34_LATTICE_POINTS];
    unsigned count = 0;
    int y;
    int x;

    if (!constellation)
        return;
    for (y = -63; y <= 65; y += 4) {
        for (x = -63; x <= 65; x += 4) {
            candidates[count].point.re = (int16_t)x;
            candidates[count].point.im = (int16_t)y;
            candidates[count].energy = (unsigned)(x * x + y * y);
            ++count;
        }
    }
    qsort(candidates, count, sizeof(candidates[0]), compare_points);
    for (count = 0; count < V34_QUARTER_POINTS; ++count)
        constellation->point[count] = candidates[count].point;
}

bool v34_constellation_point(const v34_constellation *constellation,
                             unsigned index,
                             v34_point *point)
{
    if (!constellation || !point || index >= V34_QUARTER_POINTS)
        return false;
    *point = constellation->point[index];
    return true;
}

v34_point v34_rotate_clockwise(v34_point point, unsigned turns)
{
    v34_point rotated;

    switch (turns & 3u) {
    case 0:
        return point;
    case 1:
        rotated.re = point.im;
        rotated.im = (int16_t)-point.re;
        return rotated;
    case 2:
        rotated.re = (int16_t)-point.re;
        rotated.im = (int16_t)-point.im;
        return rotated;
    default:
        rotated.re = (int16_t)-point.im;
        rotated.im = point.re;
        return rotated;
    }
}

void v34_differential_init(v34_differential_encoder *encoder)
{
    if (encoder)
        encoder->previous_rotation = 0;
}

uint8_t v34_differential_put(v34_differential_encoder *encoder,
                             uint8_t i2,
                             uint8_t i3)
{
    if (!encoder || i2 > 1u || i3 > 1u)
        return 0;
    encoder->previous_rotation =
        (uint8_t)((encoder->previous_rotation + i2 + 2u * i3) & 3u);
    return encoder->previous_rotation;
}

bool v34_map_parsed_frame(const v34_mapping_parameters *p,
                          const v34_parsed_mapping_frame *parsed,
                          bool expanded,
                          const v34_constellation *constellation,
                          v34_differential_encoder *differential,
                          v34_mapped_frame *mapped)
{
    uint8_t rings[8];
    unsigned j;
    unsigned symbol;
    unsigned bit;

    if (!p || !parsed || !constellation || !differential || !mapped ||
        parsed->shell_bit_count != p->shell_bits ||
        parsed->q_bit_count != p->q_bits ||
        !v34_shell_map(p, parsed->shell, expanded, rings))
        return false;
    memset(mapped, 0, sizeof(*mapped));

    for (j = 0; j < 4u; ++j) {
        mapped->i1[j] = parsed->i[j][0];
        mapped->z[j] = v34_differential_put(differential,
                                            parsed->i[j][1],
                                            parsed->i[j][2]);
        for (symbol = 0; symbol < 2u; ++symbol) {
            unsigned index = 0;
            unsigned ring = rings[2u * j + symbol];

            for (bit = 0; bit < p->q_bits; ++bit)
                index |= (unsigned)parsed->q[j][symbol][bit] << bit;
            index += ring << p->q_bits;
            if (index >= V34_QUARTER_POINTS ||
                !v34_constellation_point(constellation, index,
                                         &mapped->quarter_point[j][symbol]))
                return false;
            mapped->rings[j][symbol] = (uint8_t)ring;
            mapped->q_index[j][symbol] = (uint16_t)index;
        }
    }
    return true;
}
