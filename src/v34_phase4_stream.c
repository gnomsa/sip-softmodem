#include "v34_phase4_stream.h"

#include "pcma.h"

#include <string.h>

bool v34_phase4_stream_init(v34_phase4_stream *stream,
                            bool call_modem,
                            const v34_scrambler *phase3_scrambler,
                            unsigned phase3_rotation,
                            const v34_mp0 *mp,
                            v34_symbol_rate symbol_rate,
                            bool high_carrier,
                            unsigned sample_rate,
                            double amplitude)
{
    uint8_t phase;

    if (!stream)
        return false;
    memset(stream, 0, sizeof(*stream));
    if (!v34_phase4_init(&stream->phase4, call_modem,
                         phase3_scrambler, phase3_rotation, mp) ||
        !v34_symbol_clock_init(&stream->clock, symbol_rate, sample_rate) ||
        !v34_training_tx_init(&stream->tx, symbol_rate, high_carrier,
                              sample_rate, amplitude) ||
        !v34_phase4_next(&stream->phase4, &phase))
        return false;
    v34_training_tx_set_phase(&stream->tx, phase);
    return true;
}

size_t v34_phase4_stream_generate(v34_phase4_stream *stream,
                                  uint8_t *pcma, size_t count)
{
    size_t sample;

    if (!stream || !pcma)
        return 0;
    for (sample = 0; sample < count; ++sample) {
        if (stream->complete) {
            pcma[sample] = pcma_encode(0);
            continue;
        }
        pcma[sample] = v34_training_tx_pcma(&stream->tx);
        stream->samples++;
        if (v34_symbol_clock_tick(&stream->clock)) {
            uint8_t phase;
            stream->symbols++;
            if (!v34_phase4_next(&stream->phase4, &phase)) {
                stream->complete =
                    stream->phase4.state == V34_P4_COMPLETE;
                continue;
            }
            v34_training_tx_set_phase(&stream->tx, phase);
        }
    }
    return count;
}

bool v34_phase4_stream_complete(const v34_phase4_stream *stream)
{
    return stream && stream->complete && stream->symbols == 618u;
}
