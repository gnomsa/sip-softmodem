#include "v34_b1_stream.h"

#include "pcma.h"

#include <string.h>

static bool prepare_mapping_frame(v34_b1_stream *stream)
{
    v34_parsed_mapping_frame parsed;

    if (!v34_parse_mapping_frame(&stream->parameters,
                                 v34_b1_mapping_data(&stream->b1,
                                                     stream->mapping_frame),
                                 v34_b1_mapping_bits(&stream->b1,
                                                     stream->mapping_frame),
                                 &parsed))
        return false;
    return v34_map_parsed_frame(&stream->parameters, &parsed,
                                stream->expanded_shaping,
                                &stream->constellation,
                                &stream->differential, &stream->mapped);
}

static bool prepare_interval(v34_b1_stream *stream)
{
    unsigned absolute_interval =
        4u * stream->mapping_frame + stream->interval_4d;
    uint8_t v0 = (uint8_t)v34_b1_v0(&stream->b1, absolute_interval);

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

bool v34_b1_stream_init(v34_b1_stream *stream,
                        v34_symbol_rate symbol_rate,
                        unsigned data_rate,
                        bool call_modem,
                        v34_trellis_kind trellis,
                        bool expanded_shaping,
                        bool high_carrier,
                        unsigned sample_rate,
                        double coordinate_scale)
{
    if (!stream)
        return false;
    memset(stream, 0, sizeof(*stream));
    stream->expanded_shaping = expanded_shaping;
    if (!v34_b1_frame_init(&stream->b1, symbol_rate, data_rate, call_modem) ||
        !v34_mapping_parameters_init(&stream->parameters,
                                      stream->b1.geometry.high_frame_bits) ||
        !v34_trellis_init(&stream->trellis, trellis) ||
        !v34_symbol_clock_init(&stream->clock, symbol_rate, sample_rate) ||
        !v34_qam_tx_init(&stream->tx, symbol_rate, high_carrier,
                         sample_rate, coordinate_scale))
        return false;
    v34_constellation_init(&stream->constellation);
    v34_differential_init(&stream->differential);
    return prepare_mapping_frame(stream) && prepare_interval(stream);
}

static bool advance_symbol(v34_b1_stream *stream)
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
            stream->b1.geometry.mapping_frames_per_data_frame) {
            stream->complete = true;
            return true;
        }
        if (!prepare_mapping_frame(stream))
            return false;
    }
    return prepare_interval(stream);
}

size_t v34_b1_stream_generate(v34_b1_stream *stream,
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
        if (v34_symbol_clock_tick(&stream->clock) &&
            !advance_symbol(stream))
            return i + 1u;
    }
    return count;
}

bool v34_b1_stream_complete(const v34_b1_stream *stream)
{
    return stream && stream->complete;
}

uint64_t v34_b1_stream_symbols(const v34_b1_stream *stream)
{
    return stream ? stream->clock.symbols : 0;
}
