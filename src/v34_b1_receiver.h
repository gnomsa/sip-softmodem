#ifndef SOFTMODEM_V34_B1_RECEIVER_H
#define SOFTMODEM_V34_B1_RECEIVER_H

#include "v34_data_decoder.h"
#include "v34_framing.h"
#include "v34_training_rx.h"
#include "v34_training_symbols.h"

#define V34_MAX_B1_SYMBOLS (V34_MAX_MAPPING_FRAMES * 8u)

typedef struct {
    v34_frame_geometry geometry;
    v34_data_decoder decoder;
    v34_training_rx rx;
    v34_scrambler descrambler;
    v34_point received[4][2];
    v34_point expected[V34_MAX_B1_SYMBOLS];
    double coordinate_scale;
    double phase_unwrapped;
    double phase_previous;
    double phase_sum_x;
    double phase_sum_y;
    double phase_sum_xx;
    double phase_sum_xy;
    double phase_weight;
    uint64_t previous_symbol_end;
    unsigned phase_samples;
    unsigned mapping_frame;
    unsigned received_symbol;
    unsigned received_bits;
    unsigned bit_errors;
    bool complete;
    bool failed;
    bool have_phase;
} v34_b1_receiver;

bool v34_b1_receiver_init(v34_b1_receiver *receiver,
                          v34_symbol_rate symbol_rate,
                          unsigned data_rate,
                          bool call_modem,
                          v34_trellis_kind trellis,
                          bool expanded_shaping,
                          bool high_carrier,
                          unsigned sample_rate,
                          double coordinate_scale);
bool v34_b1_receiver_feed(v34_b1_receiver *receiver, uint8_t pcma);
bool v34_b1_receiver_complete(const v34_b1_receiver *receiver);

#endif
