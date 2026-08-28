#include "v34_b1.h"

bool v34_b1_frame_init(v34_b1_frame *frame,
                       v34_symbol_rate symbol_rate,
                       unsigned data_rate,
                       bool call_modem)
{
    v34_scrambler scrambler;
    unsigned mapping_frame;
    unsigned offset = 0;

    if (!frame ||
        !v34_frame_geometry_init(&frame->geometry, symbol_rate, data_rate))
        return false;

    v34_scrambler_init(&scrambler, call_modem);
    for (mapping_frame = 0;
         mapping_frame < frame->geometry.mapping_frames_per_data_frame;
         ++mapping_frame) {
        unsigned count = frame->geometry.high_frame_bits;
        unsigned bit;

        frame->mapping_offset[mapping_frame] = (uint16_t)offset;
        if (!v34_mapping_frame_high(&frame->geometry, mapping_frame))
            --count;
        for (bit = 0; bit < count; ++bit)
            frame->bits[offset++] = (uint8_t)v34_scramble_bit(&scrambler, 1u);
    }
    frame->mapping_offset[mapping_frame] = (uint16_t)offset;
    frame->scrambler_after = scrambler;
    return offset == frame->geometry.bits_per_data_frame;
}

size_t v34_b1_mapping_bits(const v34_b1_frame *frame,
                           unsigned mapping_frame)
{
    if (!frame || mapping_frame >= frame->geometry.mapping_frames_per_data_frame)
        return 0;
    return (size_t)(frame->mapping_offset[mapping_frame + 1u] -
                    frame->mapping_offset[mapping_frame]);
}

const uint8_t *v34_b1_mapping_data(const v34_b1_frame *frame,
                                   unsigned mapping_frame)
{
    if (!frame || mapping_frame >= frame->geometry.mapping_frames_per_data_frame)
        return NULL;
    return frame->bits + frame->mapping_offset[mapping_frame];
}

bool v34_b1_v0(const v34_b1_frame *frame, unsigned interval_4d)
{
    if (!frame)
        return false;
    return v34_sync_inversion(&frame->geometry,
                              frame->geometry.data_frames_per_superframe - 1u,
                              interval_4d);
}
