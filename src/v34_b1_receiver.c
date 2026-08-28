#include "v34_b1_receiver.h"

#include <string.h>

bool v34_b1_receiver_init(v34_b1_receiver *receiver,
                          v34_symbol_rate symbol_rate,
                          unsigned data_rate,
                          bool call_modem,
                          v34_trellis_kind trellis,
                          bool expanded_shaping,
                          bool high_carrier,
                          unsigned sample_rate,
                          double coordinate_scale)
{
    v34_mapping_parameters parameters;

    if (!receiver || coordinate_scale <= 0.0)
        return false;
    memset(receiver, 0, sizeof(*receiver));
    receiver->coordinate_scale = coordinate_scale;
    if (!v34_frame_geometry_init(&receiver->geometry,
                                 symbol_rate, data_rate) ||
        !v34_mapping_parameters_init(&parameters,
                                      receiver->geometry.high_frame_bits) ||
        !v34_data_decoder_init(&receiver->decoder, &parameters,
                               trellis, expanded_shaping) ||
        !v34_training_rx_init(&receiver->rx, symbol_rate,
                              high_carrier, sample_rate))
        return false;
    v34_scrambler_init(&receiver->descrambler, call_modem);
    return true;
}

static bool finish_mapping_frame(v34_b1_receiver *receiver)
{
    uint8_t v0[4];
    uint8_t scrambled[V34_MAX_DATA_FRAME_BITS];
    size_t count = 0;
    unsigned j;
    size_t bit;

    for (j = 0; j < 4u; ++j)
        v0[j] = (uint8_t)v34_sync_inversion(
            &receiver->geometry,
            receiver->geometry.data_frames_per_superframe - 1u,
            4u * receiver->mapping_frame + j);
    if (!v34_decode_mapping_frame(
            &receiver->decoder, receiver->received, v0,
            v34_mapping_frame_high(&receiver->geometry,
                                   receiver->mapping_frame),
            scrambled, &count))
        return false;
    for (bit = 0; bit < count; ++bit) {
        unsigned decoded = v34_descramble_bit(&receiver->descrambler,
                                               scrambled[bit]);
        if (decoded != 1u)
            receiver->bit_errors++;
    }
    receiver->received_bits += (unsigned)count;
    receiver->mapping_frame++;
    if (receiver->mapping_frame ==
        receiver->geometry.mapping_frames_per_data_frame) {
        receiver->complete = true;
        return receiver->received_bits == receiver->geometry.bits_per_data_frame;
    }
    return true;
}

bool v34_b1_receiver_feed(v34_b1_receiver *receiver, uint8_t pcma)
{
    double in_phase;
    double quadrature;

    if (!receiver || receiver->failed)
        return false;
    if (receiver->complete)
        return true;
    if (!v34_training_rx_pcma_iq(&receiver->rx, pcma,
                                 &in_phase, &quadrature))
        return true;
    if (!v34_slice_iq(in_phase, quadrature, receiver->coordinate_scale,
                      &receiver->received[receiver->received_symbol / 2u]
                                         [receiver->received_symbol % 2u])) {
        receiver->failed = true;
        return false;
    }
    receiver->received_symbol++;
    if (receiver->received_symbol == 8u) {
        receiver->received_symbol = 0;
        if (!finish_mapping_frame(receiver)) {
            receiver->failed = true;
            return false;
        }
    }
    return true;
}

bool v34_b1_receiver_complete(const v34_b1_receiver *receiver)
{
    return receiver && receiver->complete && !receiver->failed &&
           receiver->bit_errors == 0u;
}
