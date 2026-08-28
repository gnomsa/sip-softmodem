#include "v34_data_stream.h"

#include "pcma.h"

#include <string.h>

static bool build_data_frame(v34_data_stream *stream)
{
    unsigned offset = 0;
    unsigned mapping;

    for (mapping = 0;
         mapping < stream->geometry.mapping_frames_per_data_frame;
         ++mapping) {
        unsigned count = stream->geometry.high_frame_bits;
        unsigned bit;

        stream->mapping_offset[mapping] = (uint16_t)offset;
        if (!v34_mapping_frame_high(&stream->geometry, mapping))
            --count;
        for (bit = 0; bit < count; ++bit) {
            if (stream->input_offset >= stream->input_count)
                return false;
            stream->frame_bits[offset++] = (uint8_t)v34_scramble_bit(
                &stream->scrambler,
                stream->input_bits[stream->input_offset++]);
        }
    }
    stream->mapping_offset[mapping] = (uint16_t)offset;
    return offset == stream->geometry.bits_per_data_frame;
}

static bool prepare_mapping_frame(v34_data_stream *stream)
{
    v34_parsed_mapping_frame parsed;
    size_t offset = stream->mapping_offset[stream->mapping_frame];
    size_t count = stream->mapping_offset[stream->mapping_frame + 1u] - offset;

    if (!v34_parse_mapping_frame(&stream->parameters,
                                 stream->frame_bits + offset, count, &parsed))
        return false;
    return v34_map_parsed_frame(&stream->parameters, &parsed,
                                stream->expanded_shaping,
                                &stream->constellation,
                                &stream->differential, &stream->mapped);
}

static bool prepare_interval(v34_data_stream *stream)
{
    unsigned absolute_interval =
        4u * stream->mapping_frame + stream->interval_4d;
    uint8_t v0 = (uint8_t)v34_sync_inversion(&stream->geometry,
                                              stream->data_frame,
                                              absolute_interval);

    if (!v34_encode_4d_zero_precoder(&stream->trellis,
            stream->mapped.quarter_point[stream->interval_4d][0],
            stream->mapped.quarter_point[stream->interval_4d][1],
            stream->mapped.z[stream->interval_4d],
            stream->mapped.i1[stream->interval_4d], v0,
            stream->interval))
        return false;
    stream->symbol_in_4d = 0;
    v34_qam_tx_set_point(&stream->tx, stream->interval[0]);
    return true;
}

bool v34_data_stream_init_after_b1(v34_data_stream *stream,
                                   const v34_b1_stream *b1,
                                   const uint8_t *input_bits,
                                   size_t input_count)
{
    size_t expected;
    size_t bit;

    if (!stream || !b1 || !input_bits || !v34_b1_stream_complete(b1))
        return false;
    expected = (size_t)b1->b1.geometry.bits_per_data_frame *
               b1->b1.geometry.data_frames_per_superframe;
    if (input_count != expected || input_count > V34_MAX_SUPERFRAME_BITS)
        return false;
    for (bit = 0; bit < input_count; ++bit)
        if (input_bits[bit] > 1u)
            return false;

    memset(stream, 0, sizeof(*stream));
    stream->geometry = b1->b1.geometry;
    stream->parameters = b1->parameters;
    stream->constellation = b1->constellation;
    stream->differential = b1->differential;
    stream->trellis = b1->trellis;
    stream->scrambler = b1->b1.scrambler_after;
    stream->clock = b1->clock;
    stream->tx = b1->tx;
    stream->expanded_shaping = b1->expanded_shaping;
    stream->input_count = input_count;
    memcpy(stream->input_bits, input_bits, input_count);
    return build_data_frame(stream) && prepare_mapping_frame(stream) &&
           prepare_interval(stream);
}

bool v34_data_stream_next_superframe(v34_data_stream *stream,
                                     const uint8_t *input_bits,
                                     size_t input_count)
{
    size_t expected;
    size_t bit;

    if (!stream || !input_bits || !stream->complete)
        return false;
    expected = (size_t)stream->geometry.bits_per_data_frame *
               stream->geometry.data_frames_per_superframe;
    if (input_count != expected || input_count > V34_MAX_SUPERFRAME_BITS)
        return false;
    for (bit = 0; bit < input_count; ++bit)
        if (input_bits[bit] > 1u)
            return false;

    stream->input_count = input_count;
    stream->input_offset = 0;
    stream->data_frame = 0;
    stream->mapping_frame = 0;
    stream->interval_4d = 0;
    stream->symbol_in_4d = 0;
    stream->complete = false;
    memcpy(stream->input_bits, input_bits, input_count);
    return build_data_frame(stream) && prepare_mapping_frame(stream) &&
           prepare_interval(stream);
}

static bool advance_symbol(v34_data_stream *stream)
{
    if (stream->symbol_in_4d == 0u) {
        stream->symbol_in_4d = 1u;
        v34_qam_tx_set_point(&stream->tx, stream->interval[1]);
        return true;
    }
    stream->interval_4d++;
    if (stream->interval_4d == 4u) {
        stream->interval_4d = 0;
        stream->mapping_frame++;
        if (stream->mapping_frame ==
            stream->geometry.mapping_frames_per_data_frame) {
            stream->mapping_frame = 0;
            stream->data_frame++;
            if (stream->data_frame ==
                stream->geometry.data_frames_per_superframe) {
                stream->complete = true;
                return true;
            }
            if (!build_data_frame(stream))
                return false;
        }
        if (!prepare_mapping_frame(stream))
            return false;
    }
    return prepare_interval(stream);
}

size_t v34_data_stream_generate(v34_data_stream *stream,
                                uint8_t *pcma,
                                size_t count)
{
    size_t i;

    if (!stream || !pcma)
        return 0;
    for (i = 0; i < count; ++i) {
        if (stream->complete) {
            pcma[i] = pcma_encode(0);
            continue;
        }
        pcma[i] = v34_qam_tx_pcma(&stream->tx);
        stream->active_samples++;
        if (v34_symbol_clock_tick(&stream->clock)) {
            stream->symbols++;
            if (!advance_symbol(stream))
                return i + 1u;
        }
    }
    return count;
}

bool v34_data_stream_complete(const v34_data_stream *stream)
{
    return stream && stream->complete;
}
