#include "v34_data_receiver.h"

#include <math.h>
#include <string.h>

static bool finish_mapping_frame(v34_data_receiver *receiver)
{
    uint8_t v0[4];
    uint8_t scrambled[V34_MAX_DATA_FRAME_BITS];
    v34_point hard[4][2];
    size_t scrambled_count = 0;
    size_t bit;
    unsigned j;

    for (j = 0; j < 4u; ++j)
        v0[j] = (uint8_t)v34_sync_inversion(
            &receiver->geometry, receiver->data_frame,
            4u * receiver->mapping_frame + j);
    memcpy(hard, receiver->received, sizeof(hard));
    if (!v34_decode_mapping_frame_soft(
            &receiver->decoder, receiver->received_iq,
            receiver->coordinate_scale, v0,
            v34_mapping_frame_high(&receiver->geometry,
                                   receiver->mapping_frame),
            scrambled, &scrambled_count, receiver->received) ||
        receiver->output_count + scrambled_count > V34_MAX_SUPERFRAME_BITS)
        return false;
    for (j = 0; j < 4u; ++j) {
        unsigned symbol;
        for (symbol = 0; symbol < 2u; ++symbol)
            if (hard[j][symbol].re != receiver->received[j][symbol].re ||
                hard[j][symbol].im != receiver->received[j][symbol].im)
                receiver->soft_corrections++;
    }
    for (bit = 0; bit < scrambled_count; ++bit)
        receiver->output_bits[receiver->output_count++] =
            (uint8_t)v34_descramble_bit(&receiver->descrambler,
                                        scrambled[bit]);

    receiver->mapping_frame++;
    if (receiver->mapping_frame ==
        receiver->geometry.mapping_frames_per_data_frame) {
        receiver->mapping_frame = 0;
        receiver->data_frame++;
        if (receiver->data_frame ==
            receiver->geometry.data_frames_per_superframe) {
            size_t expected =
                (size_t)receiver->geometry.bits_per_data_frame *
                receiver->geometry.data_frames_per_superframe;
            receiver->complete = receiver->output_count == expected;
            return receiver->complete;
        }
    }
    return true;
}

bool v34_data_receiver_init_after_b1(v34_data_receiver *receiver,
                                     const v34_b1_receiver *b1)
{
    if (!receiver || !v34_b1_receiver_complete(b1))
        return false;
    memset(receiver, 0, sizeof(*receiver));
    receiver->geometry = b1->geometry;
    receiver->decoder = b1->decoder;
    receiver->rx = b1->rx;
    receiver->descrambler = b1->descrambler;
    receiver->coordinate_scale = b1->coordinate_scale;
    receiver->track_carrier = fabs(b1->frequency_error) >= 2e-5;
    return true;
}

bool v34_data_receiver_next_superframe(v34_data_receiver *receiver)
{
    if (!v34_data_receiver_complete(receiver) ||
        receiver->read_offset != receiver->output_count)
        return false;
    receiver->output_count = 0;
    receiver->read_offset = 0;
    receiver->data_frame = 0;
    receiver->mapping_frame = 0;
    receiver->received_symbol = 0;
    receiver->complete = false;
    return true;
}

static bool feed_sample(v34_data_receiver *receiver, uint8_t pcma)
{
    double in_phase;
    double quadrature;
    v34_point *point;

    if (!v34_training_rx_pcma_iq(&receiver->rx, pcma,
                                 &in_phase, &quadrature))
        return true;
    point = &receiver->received[receiver->received_symbol / 2u]
                               [receiver->received_symbol % 2u];
    receiver->received_iq[receiver->received_symbol / 2u]
                         [receiver->received_symbol % 2u][0] = in_phase;
    receiver->received_iq[receiver->received_symbol / 2u]
                         [receiver->received_symbol % 2u][1] = quadrature;
    if (!v34_slice_iq(in_phase, quadrature,
                      receiver->coordinate_scale, point))
        return false;
    if (receiver->track_carrier)
        v34_training_rx_track_carrier(
            &receiver->rx, in_phase, quadrature,
            receiver->coordinate_scale * point->re,
            receiver->coordinate_scale * point->im);
    receiver->received_symbol++;
    receiver->symbols++;
    if (receiver->received_symbol != 8u)
        return true;
    receiver->received_symbol = 0;
    return finish_mapping_frame(receiver);
}

bool v34_data_receiver_feed(v34_data_receiver *receiver,
                            const uint8_t *pcma, size_t count)
{
    size_t sample;

    if (!receiver || (!pcma && count != 0u) || receiver->failed)
        return false;
    for (sample = 0; sample < count && !receiver->complete; ++sample) {
        if (!feed_sample(receiver, pcma[sample])) {
            receiver->failed = true;
            return false;
        }
    }
    return true;
}

size_t v34_data_receiver_read(v34_data_receiver *receiver,
                              uint8_t *bits, size_t capacity)
{
    size_t available;

    if (!receiver || (!bits && capacity != 0u))
        return 0;
    available = receiver->output_count - receiver->read_offset;
    if (capacity > available)
        capacity = available;
    if (capacity != 0u) {
        memcpy(bits, receiver->output_bits + receiver->read_offset, capacity);
        receiver->read_offset += capacity;
    }
    return capacity;
}

bool v34_data_receiver_complete(const v34_data_receiver *receiver)
{
    return receiver && receiver->complete && !receiver->failed;
}
