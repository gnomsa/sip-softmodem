#ifndef SOFTMODEM_V34_B1_DATA_LINK_H
#define SOFTMODEM_V34_B1_DATA_LINK_H

#include "v34_data_channel.h"
#include "v34_phase4_receiver.h"
#include "v34_phase4_stream.h"

typedef struct {
    v34_b1_stream tx_b1;
    v34_b1_receiver rx_b1;
    v34_data_channel data;
    uint8_t pending[V34_UART_QUEUE_SIZE];
    size_t pending_head;
    size_t pending_tail;
    uint64_t packets;
    bool rx_b1_started;
    bool rx_data_started;
    bool data_ready;
    bool failed;
} v34_b1_data_link;

bool v34_b1_data_link_init(v34_b1_data_link *link,
                           bool call_modem,
                           v34_symbol_rate tx_symbol_rate,
                           unsigned tx_data_rate,
                           v34_symbol_rate rx_symbol_rate,
                           unsigned rx_data_rate,
                           v34_trellis_kind trellis,
                           bool expanded_shaping,
                           unsigned sample_rate,
                           double coordinate_scale);
bool v34_b1_data_link_init_after_phase4(
    v34_b1_data_link *link,
    bool call_modem,
    const v34_phase4_stream *transmitted_phase4,
    const v34_phase4_receiver *received_phase4,
    v34_symbol_rate tx_symbol_rate,
    unsigned tx_data_rate,
    v34_symbol_rate rx_symbol_rate,
    unsigned rx_data_rate,
    v34_trellis_kind trellis,
    bool expanded_shaping,
    unsigned sample_rate,
    double coordinate_scale);
size_t v34_b1_data_link_write(v34_b1_data_link *link,
                              const uint8_t *bytes, size_t count);
size_t v34_b1_data_link_read(v34_b1_data_link *link,
                             uint8_t *bytes, size_t capacity);
bool v34_b1_data_link_generate(
    v34_b1_data_link *link,
    uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
bool v34_b1_data_link_receive(
    v34_b1_data_link *link,
    const uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
bool v34_b1_data_link_connected(const v34_b1_data_link *link);

#endif
