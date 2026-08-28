#ifndef SOFTMODEM_V34_DATA_CHANNEL_H
#define SOFTMODEM_V34_DATA_CHANNEL_H

#include "v34_data_receiver.h"
#include "v34_data_stream.h"
#include "v34_uart.h"

#define V34_PCMA_PACKET_SAMPLES 160u

typedef struct {
    v34_b1_stream tx_b1;
    v34_data_stream tx;
    v34_data_receiver rx;
    v34_uart uart;
    uint8_t tx_bits[V34_MAX_SUPERFRAME_BITS];
    uint8_t rx_bits[V34_MAX_SUPERFRAME_BITS];
    size_t superframe_bits;
    uint64_t tx_packets;
    uint64_t rx_packets;
    bool tx_started;
    bool failed;
} v34_data_channel;

bool v34_data_channel_init_after_b1(v34_data_channel *channel,
                                    const v34_b1_stream *transmitted_b1,
                                    const v34_b1_receiver *received_b1);
size_t v34_data_channel_write(v34_data_channel *channel,
                              const uint8_t *bytes, size_t count);
size_t v34_data_channel_read(v34_data_channel *channel,
                             uint8_t *bytes, size_t capacity);
bool v34_data_channel_generate(v34_data_channel *channel,
                               uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);
bool v34_data_channel_receive(v34_data_channel *channel,
                              const uint8_t pcma[V34_PCMA_PACKET_SAMPLES]);

#endif
