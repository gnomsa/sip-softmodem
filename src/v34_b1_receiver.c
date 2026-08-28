#include "v34_b1_receiver.h"

#include "v34_b1.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static bool build_expected(v34_b1_receiver *receiver,
                           v34_symbol_rate symbol_rate,
                           unsigned data_rate,
                           bool call_modem,
                           v34_trellis_kind kind,
                           bool expanded)
{
    v34_b1_frame b1;
    v34_mapping_parameters parameters;
    v34_constellation constellation;
    v34_differential_encoder differential;
    v34_trellis_encoder trellis;
    unsigned mapping;
    unsigned output = 0;

    if (!v34_b1_frame_init(&b1, symbol_rate, data_rate, call_modem) ||
        !v34_mapping_parameters_init(&parameters,
                                      b1.geometry.high_frame_bits) ||
        !v34_trellis_init(&trellis, kind))
        return false;
    v34_constellation_init(&constellation);
    v34_differential_init(&differential);
    for (mapping = 0;
         mapping < b1.geometry.mapping_frames_per_data_frame;
         ++mapping) {
        v34_parsed_mapping_frame parsed;
        v34_mapped_frame mapped;
        unsigned j;

        if (!v34_parse_mapping_frame(&parameters,
                                     v34_b1_mapping_data(&b1, mapping),
                                     v34_b1_mapping_bits(&b1, mapping),
                                     &parsed) ||
            !v34_map_parsed_frame(&parameters, &parsed, expanded,
                                  &constellation, &differential, &mapped))
            return false;
        for (j = 0; j < 4u; ++j) {
            v34_point pair[2];
            uint8_t v0 = (uint8_t)v34_b1_v0(&b1, 4u * mapping + j);
            if (!v34_encode_4d_zero_precoder(&trellis,
                    mapped.quarter_point[j][0], mapped.quarter_point[j][1],
                    mapped.z[j], mapped.i1[j], v0, pair) ||
                output + 2u > V34_MAX_B1_SYMBOLS)
                return false;
            receiver->expected[output++] = pair[0];
            receiver->expected[output++] = pair[1];
        }
    }
    return output == 8u * b1.geometry.mapping_frames_per_data_frame;
}

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
                              high_carrier, sample_rate) ||
        !build_expected(receiver, symbol_rate, data_rate, call_modem,
                        trellis, expanded_shaping))
        return false;
    v34_scrambler_init(&receiver->descrambler, call_modem);
    return true;
}

static void correct_known_symbol(v34_b1_receiver *receiver,
                                 double *in_phase, double *quadrature)
{
    v34_point expected = receiver->expected[
        8u * receiver->mapping_frame + receiver->received_symbol];
    double dot = *in_phase * expected.re + *quadrature * expected.im;
    double cross = *quadrature * expected.re - *in_phase * expected.im;
    double phase = atan2(cross, dot);
    double cosine = cos(phase);
    double sine = sin(phase);
    double corrected_i = *in_phase * cosine + *quadrature * sine;
    double corrected_q = *quadrature * cosine - *in_phase * sine;
    double weight = (double)expected.re * expected.re +
                    (double)expected.im * expected.im;
    double symbol_end = (double)receiver->rx.clock.samples;
    double symbol_center =
        ((double)receiver->previous_symbol_end + symbol_end - 1.0) / 2.0;

    if (!receiver->have_phase) {
        receiver->phase_unwrapped = phase;
        receiver->have_phase = true;
    } else {
        double change = phase - receiver->phase_previous;
        while (change > M_PI)
            change -= 2.0 * M_PI;
        while (change < -M_PI)
            change += 2.0 * M_PI;
        receiver->phase_unwrapped += change;
    }
    receiver->phase_previous = phase;
    receiver->phase_sum_x += weight * symbol_center;
    receiver->phase_sum_y += weight * receiver->phase_unwrapped;
    receiver->phase_sum_xx += weight * symbol_center * symbol_center;
    receiver->phase_sum_xy +=
        weight * symbol_center * receiver->phase_unwrapped;
    receiver->phase_weight += weight;
    receiver->phase_samples++;
    receiver->previous_symbol_end = receiver->rx.clock.samples;
    *in_phase = corrected_i;
    *quadrature = corrected_q;
}

static void finish_carrier_estimate(v34_b1_receiver *receiver)
{
    double weight = receiver->phase_weight;
    double denominator;
    double frequency_error = 0.0;
    double phase_intercept;
    double phase_at_boundary;

    if (!receiver->have_phase)
        return;
    denominator = weight * receiver->phase_sum_xx -
                  receiver->phase_sum_x * receiver->phase_sum_x;
    if (receiver->phase_samples > 1u && denominator > 0.0)
        frequency_error =
            (weight * receiver->phase_sum_xy -
             receiver->phase_sum_x * receiver->phase_sum_y) / denominator;
    phase_intercept =
        (receiver->phase_sum_y - frequency_error * receiver->phase_sum_x) /
        weight;
    phase_at_boundary = phase_intercept +
        frequency_error * (double)receiver->rx.clock.samples;
    /* PCMA quantisation and the short rectangular symbol integrator leave a
     * minute deterministic fit error even when both oscillators are equal.
     * Do not turn that estimator floor into a growing data-mode phase error. */
    if (fabs(frequency_error) < 2e-5 && fabs(phase_at_boundary) < 1e-2) {
        frequency_error = 0.0;
        phase_at_boundary = 0.0;
    }
    receiver->rx.carrier_step += frequency_error;
    receiver->rx.carrier_phase =
        fmod(receiver->rx.carrier_phase + phase_at_boundary, 2.0 * M_PI);
    if (receiver->rx.carrier_phase < 0.0)
        receiver->rx.carrier_phase += 2.0 * M_PI;
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
    correct_known_symbol(receiver, &in_phase, &quadrature);
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
        if (receiver->complete)
            finish_carrier_estimate(receiver);
    }
    return true;
}

bool v34_b1_receiver_complete(const v34_b1_receiver *receiver)
{
    return receiver && receiver->complete && !receiver->failed &&
           receiver->bit_errors == 0u;
}
