#ifndef SOFTMODEM_V34_DATA_MAPPER_H
#define SOFTMODEM_V34_DATA_MAPPER_H

#include "v34_mapper.h"

#define V34_QUARTER_POINTS 416u

typedef struct {
    int16_t re;
    int16_t im;
} v34_point;

typedef struct {
    v34_point point[V34_QUARTER_POINTS];
} v34_constellation;

typedef struct {
    uint8_t previous_rotation;
} v34_differential_encoder;

typedef struct {
    uint8_t rings[4][2];
    uint16_t q_index[4][2];
    v34_point quarter_point[4][2];
    uint8_t z[4];
    uint8_t i1[4];
} v34_mapped_frame;

void v34_constellation_init(v34_constellation *constellation);
bool v34_constellation_point(const v34_constellation *constellation,
                             unsigned index,
                             v34_point *point);
v34_point v34_rotate_clockwise(v34_point point, unsigned quarter_turns);
void v34_differential_init(v34_differential_encoder *encoder);
uint8_t v34_differential_put(v34_differential_encoder *encoder,
                             uint8_t i2,
                             uint8_t i3);
bool v34_map_parsed_frame(const v34_mapping_parameters *parameters,
                          const v34_parsed_mapping_frame *parsed,
                          bool expanded_shaping,
                          const v34_constellation *constellation,
                          v34_differential_encoder *differential,
                          v34_mapped_frame *mapped);

#endif
