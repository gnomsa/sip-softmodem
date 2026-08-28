#include "v34_data_decoder.h"

#include "v34_framing.h"

#include <math.h>
#include <string.h>

static bool quarter_residue(v34_point point)
{
    int re = (point.re - 1) % 4;
    int im = (point.im - 1) % 4;
    return re == 0 && im == 0;
}

bool v34_quarter_decode(const v34_constellation *constellation,
                        v34_point point,
                        uint8_t *rotation,
                        uint16_t *index)
{
    unsigned turn;

    if (!constellation || !rotation || !index)
        return false;
    for (turn = 0; turn < 4u; ++turn) {
        v34_point quarter = v34_rotate_clockwise(point, 4u - turn);
        unsigned candidate;

        if (!quarter_residue(quarter))
            continue;
        for (candidate = 0; candidate < V34_QUARTER_POINTS; ++candidate)
            if (constellation->point[candidate].re == quarter.re &&
                constellation->point[candidate].im == quarter.im) {
                *rotation = (uint8_t)turn;
                *index = (uint16_t)candidate;
                return true;
            }
    }
    return false;
}

bool v34_data_decoder_init(v34_data_decoder *decoder,
                           const v34_mapping_parameters *parameters,
                           v34_trellis_kind trellis,
                           bool expanded_shaping)
{
    if (!decoder || !parameters ||
        !v34_trellis_init(&decoder->trellis, trellis))
        return false;
    decoder->parameters = *parameters;
    decoder->expanded_shaping = expanded_shaping;
    v34_constellation_init(&decoder->constellation);
    v34_differential_init(&decoder->differential);
    return true;
}

bool v34_slice_iq(double in_phase, double quadrature,
                  double scale, v34_point *point)
{
    long re;
    long im;

    if (!point || scale <= 0.0 || !isfinite(in_phase) ||
        !isfinite(quadrature))
        return false;
    re = 2l * lround((in_phase / scale - 1.0) / 2.0) + 1l;
    im = 2l * lround((quadrature / scale - 1.0) / 2.0) + 1l;
    if (re < INT16_MIN || re > INT16_MAX ||
        im < INT16_MIN || im > INT16_MAX)
        return false;
    point->re = (int16_t)re;
    point->im = (int16_t)im;
    return true;
}

static bool decode_intervals(v34_data_decoder *decoder,
                             v34_point received[4][2],
                             const uint8_t v0[4],
                             v34_parsed_mapping_frame *parsed,
                             uint8_t rings[8])
{
    unsigned j;

    memset(parsed, 0, sizeof(*parsed));
    parsed->shell_bit_count = decoder->parameters.shell_bits;
    parsed->q_bit_count = decoder->parameters.q_bits;
    for (j = 0; j < 4u; ++j) {
        uint8_t first_rotation;
        uint8_t second_rotation;
        uint16_t first_index;
        uint16_t second_index;
        uint8_t first_label;
        uint8_t second_label;
        uint8_t y4, y3, y2, y1;
        uint8_t u0 = (uint8_t)(v34_trellis_output(&decoder->trellis) ^
                               v0[j]);
        unsigned delta;
        unsigned differential;
        unsigned bit;

        if (!v34_quarter_decode(&decoder->constellation, received[j][0],
                                &first_rotation, &first_index) ||
            !v34_quarter_decode(&decoder->constellation, received[j][1],
                                &second_rotation, &second_index))
            return false;
        delta = (second_rotation + 4u - first_rotation - u0) & 3u;
        if (delta != 0u && delta != 2u)
            return false;
        parsed->i[j][0] = (uint8_t)(delta >> 1u);
        differential = (first_rotation + 4u -
                        decoder->differential.previous_rotation) & 3u;
        parsed->i[j][1] = (uint8_t)(differential & 1u);
        parsed->i[j][2] = (uint8_t)((differential >> 1u) & 1u);
        decoder->differential.previous_rotation = first_rotation;

        rings[2u * j] = (uint8_t)(first_index >> decoder->parameters.q_bits);
        rings[2u * j + 1u] =
            (uint8_t)(second_index >> decoder->parameters.q_bits);
        for (bit = 0; bit < decoder->parameters.q_bits; ++bit) {
            parsed->q[j][0][bit] = (uint8_t)((first_index >> bit) & 1u);
            parsed->q[j][1][bit] = (uint8_t)((second_index >> bit) & 1u);
        }

        if (!v34_subset_label(received[j][0], &first_label) ||
            !v34_subset_label(received[j][1], &second_label) ||
            !v34_subset_pair_bits(first_label, second_label,
                                  &y4, &y3, &y2, &y1))
            return false;
        (void)v34_trellis_put(&decoder->trellis, y4, y3, y2, y1);
    }
    return v34_shell_unmap(&decoder->parameters, rings,
                           decoder->expanded_shaping, parsed->shell);
}

static bool emit_large(const v34_mapping_parameters *p,
                       const v34_parsed_mapping_frame *parsed,
                       bool high, uint8_t *bits, size_t *count)
{
    size_t offset = 0;
    unsigned bit;
    unsigned j;
    unsigned symbol;

    if (!high && parsed->shell[p->shell_bits - 1u] != 0u)
        return false;
    for (bit = 0; bit < p->shell_bits - (high ? 0u : 1u); ++bit)
        bits[offset++] = parsed->shell[bit];
    for (j = 0; j < 4u; ++j) {
        for (bit = 0; bit < 3u; ++bit)
            bits[offset++] = parsed->i[j][bit];
        for (symbol = 0; symbol < 2u; ++symbol)
            for (bit = 0; bit < p->q_bits; ++bit)
                bits[offset++] = parsed->q[j][symbol][bit];
    }
    *count = offset;
    return offset == p->frame_bits - (high ? 0u : 1u);
}

static bool emit_small(const v34_mapping_parameters *p,
                       const v34_parsed_mapping_frame *parsed,
                       bool high, uint8_t *bits, size_t *count)
{
    unsigned transmitted = p->frame_bits - (high ? 0u : 1u);
    unsigned full_groups;
    unsigned j;
    size_t offset = 0;

    if (transmitted == 8u)
        full_groups = 0;
    else if (transmitted == 9u)
        full_groups = 1;
    else if (transmitted == 11u)
        full_groups = 3;
    else if (transmitted == 12u)
        full_groups = 4;
    else
        return false;
    for (j = 0; j < 4u; ++j) {
        bits[offset++] = parsed->i[j][0];
        bits[offset++] = parsed->i[j][1];
        if (j < full_groups)
            bits[offset++] = parsed->i[j][2];
        else if (parsed->i[j][2] != 0u)
            return false;
    }
    *count = offset;
    return offset == transmitted;
}

bool v34_decode_mapping_frame(v34_data_decoder *decoder,
                              v34_point received[4][2],
                              const uint8_t v0[4],
                              bool high_frame,
                              uint8_t *bits,
                              size_t *count)
{
    v34_parsed_mapping_frame parsed;
    uint8_t rings[8];

    if (!decoder || !received || !v0 || !bits || !count ||
        !decode_intervals(decoder, received, v0, &parsed, rings))
        return false;
    parsed.low_frame = !high_frame;
    if (decoder->parameters.frame_bits <= 12u)
        return emit_small(&decoder->parameters, &parsed,
                          high_frame, bits, count);
    return emit_large(&decoder->parameters, &parsed,
                      high_frame, bits, count);
}

#define SOFT_CANDIDATES 9u
#define SOFT_PATHS_PER_STATE 2u
#define SOFT_STATE_KEYS (64u * 4u)

typedef struct {
    v34_point point;
    double metric;
    uint8_t rotation;
    uint8_t label;
} soft_candidate;

typedef struct {
    v34_point point[4][2];
    double metric;
    uint8_t trellis_state;
    bool used;
} soft_path;

static unsigned make_candidates(const v34_data_decoder *decoder,
                                double in_phase, double quadrature,
                                double scale,
                                soft_candidate candidates[SOFT_CANDIDATES])
{
    v34_point nearest;
    unsigned count = 0;
    int dy;
    int dx;

    if (!v34_slice_iq(in_phase, quadrature, scale, &nearest))
        return 0;
    for (dy = -2; dy <= 2; dy += 2) {
        for (dx = -2; dx <= 2; dx += 2) {
            v34_point point = {
                (int16_t)(nearest.re + dx),
                (int16_t)(nearest.im + dy)
            };
            uint16_t index;
            double error_i;
            double error_q;

            if (!v34_quarter_decode(&decoder->constellation, point,
                                    &candidates[count].rotation, &index) ||
                !v34_subset_label(point, &candidates[count].label))
                continue;
            error_i = in_phase / scale - point.re;
            error_q = quadrature / scale - point.im;
            candidates[count].point = point;
            candidates[count].metric =
                error_i * error_i + error_q * error_q;
            count++;
        }
    }
    return count;
}

static void keep_path(soft_path paths[SOFT_PATHS_PER_STATE],
                      const soft_path *candidate)
{
    unsigned slot;
    unsigned worst = 0;

    for (slot = 0; slot < SOFT_PATHS_PER_STATE; ++slot) {
        if (!paths[slot].used) {
            paths[slot] = *candidate;
            return;
        }
        if (paths[slot].metric > paths[worst].metric)
            worst = slot;
    }
    if (candidate->metric < paths[worst].metric)
        paths[worst] = *candidate;
}

bool v34_decode_mapping_frame_soft(v34_data_decoder *decoder,
                                   double received_iq[4][2][2],
                                   double scale,
                                   const uint8_t v0[4],
                                   bool high_frame,
                                   uint8_t *bits,
                                   size_t *count,
                                   v34_point decided[4][2])
{
    soft_path current[SOFT_STATE_KEYS][SOFT_PATHS_PER_STATE] = {0};
    soft_path next[SOFT_STATE_KEYS][SOFT_PATHS_PER_STATE];
    unsigned initial_key;
    unsigned interval;
    double best_metric = HUGE_VAL;
    v34_data_decoder original;
    v34_data_decoder best_decoder;
    bool found = false;

    if (!decoder || !received_iq || scale <= 0.0 || !v0 || !bits ||
        !count || !decided)
        return false;
    original = *decoder;
    initial_key = 4u * decoder->trellis.state +
                  decoder->differential.previous_rotation;
    current[initial_key][0].used = true;
    current[initial_key][0].trellis_state = decoder->trellis.state;

    for (interval = 0; interval < 4u; ++interval) {
        soft_candidate first[SOFT_CANDIDATES];
        soft_candidate second[SOFT_CANDIDATES];
        unsigned first_count = make_candidates(
            decoder, received_iq[interval][0][0],
            received_iq[interval][0][1], scale, first);
        unsigned second_count = make_candidates(
            decoder, received_iq[interval][1][0],
            received_iq[interval][1][1], scale, second);
        unsigned key;

        if (first_count == 0u || second_count == 0u)
            return false;
        memset(next, 0, sizeof(next));
        for (key = 0; key < SOFT_STATE_KEYS; ++key) {
            unsigned slot;
            for (slot = 0; slot < SOFT_PATHS_PER_STATE; ++slot) {
                const soft_path *path = &current[key][slot];
                unsigned a;
                if (!path->used)
                    continue;
                for (a = 0; a < first_count; ++a) {
                    unsigned b;
                    for (b = 0; b < second_count; ++b) {
                        v34_trellis_encoder trellis = decoder->trellis;
                        uint8_t y4, y3, y2, y1;
                        uint8_t u0;
                        unsigned delta;
                        unsigned next_key;
                        soft_path branch;

                        trellis.state = path->trellis_state;
                        u0 = (uint8_t)(v34_trellis_output(&trellis) ^
                                       v0[interval]);
                        delta = (second[b].rotation + 4u -
                                 first[a].rotation - u0) & 3u;
                        if (delta != 0u && delta != 2u)
                            continue;
                        if (!v34_subset_pair_bits(
                                first[a].label, second[b].label,
                                &y4, &y3, &y2, &y1))
                            continue;
                        (void)v34_trellis_put(&trellis, y4, y3, y2, y1);
                        branch = *path;
                        branch.point[interval][0] = first[a].point;
                        branch.point[interval][1] = second[b].point;
                        branch.metric += first[a].metric + second[b].metric;
                        branch.trellis_state = trellis.state;
                        branch.used = true;
                        next_key = 4u * trellis.state + first[a].rotation;
                        keep_path(next[next_key], &branch);
                    }
                }
            }
        }
        memcpy(current, next, sizeof(current));
    }

    {
        unsigned key;
        for (key = 0; key < SOFT_STATE_KEYS; ++key) {
            unsigned slot;
            for (slot = 0; slot < SOFT_PATHS_PER_STATE; ++slot) {
                soft_path *path = &current[key][slot];
                v34_data_decoder trial;
                uint8_t trial_bits[V34_MAX_DATA_FRAME_BITS];
                size_t trial_count = 0;

                if (!path->used || path->metric >= best_metric)
                    continue;
                trial = original;
                if (!v34_decode_mapping_frame(
                        &trial, path->point, v0, high_frame,
                        trial_bits, &trial_count))
                    continue;
                best_metric = path->metric;
                best_decoder = trial;
                *count = trial_count;
                memcpy(bits, trial_bits, trial_count);
                memcpy(decided, path->point, sizeof(path->point));
                found = true;
            }
        }
    }
    if (found)
        *decoder = best_decoder;
    return found;
}
