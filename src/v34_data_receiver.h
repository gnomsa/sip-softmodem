#ifndef SOFTMODEM_V34_DATA_RECEIVER_H
#define SOFTMODEM_V34_DATA_RECEIVER_H

#include "v34_b1_receiver.h"

typedef struct {
    v34_frame_geometry geometry;
    v34_data_decoder decoder;
    v34_training_rx rx;
    v34_scrambler descrambler;
    v34_point received[4][2];
    double received_iq[4][2][2];
    uint8_t output_bits[V34_MAX_SUPERFRAME_BITS];
    double coordinate_scale;
    size_t output_count;
    size_t read_offset;
    unsigned data_frame;
    unsigned mapping_frame;
    unsigned received_symbol;
    uint64_t symbols;
    uint64_t soft_corrections;
    bool complete;
    bool failed;
    bool track_carrier;
} v34_data_receiver;

bool v34_data_receiver_init_after_b1(v34_data_receiver *receiver,
                                     const v34_b1_receiver *b1);
bool v34_data_receiver_next_superframe(v34_data_receiver *receiver);
bool v34_data_receiver_feed(v34_data_receiver *receiver,
                            const uint8_t *pcma, size_t count);
size_t v34_data_receiver_read(v34_data_receiver *receiver,
                              uint8_t *bits, size_t capacity);
bool v34_data_receiver_complete(const v34_data_receiver *receiver);

#endif
