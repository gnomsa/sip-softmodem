#ifndef SOFTMODEM_V34_MAPPER_H
#define SOFTMODEM_V34_MAPPER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V34_MAX_SHELL_BITS 31u
#define V34_MAX_Q_BITS 5u

typedef struct {
    unsigned frame_bits;
    unsigned shell_bits;
    unsigned q_bits;
    unsigned minimum_rings;
    unsigned expanded_rings;
    unsigned minimum_points;
    unsigned expanded_points;
} v34_mapping_parameters;

typedef struct {
    bool low_frame;
    unsigned shell_bit_count;
    unsigned q_bit_count;
    uint8_t shell[V34_MAX_SHELL_BITS];
    uint8_t i[4][3];
    uint8_t q[4][2][V34_MAX_Q_BITS];
} v34_parsed_mapping_frame;

bool v34_mapping_parameters_init(v34_mapping_parameters *parameters,
                                 unsigned high_frame_bits);
bool v34_parse_mapping_frame(const v34_mapping_parameters *parameters,
                             const uint8_t *bits,
                             size_t bit_count,
                             v34_parsed_mapping_frame *parsed);
bool v34_shell_map(const v34_mapping_parameters *parameters,
                   const uint8_t *shell_bits,
                   bool expanded_shaping,
                   uint8_t rings[8]);

#endif
