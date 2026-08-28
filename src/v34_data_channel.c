#include "v34_data_channel.h"

#include <string.h>

bool v34_data_channel_init_after_b1(v34_data_channel *channel,
                                    const v34_b1_stream *transmitted_b1,
                                    const v34_b1_receiver *received_b1)
{
    if (!channel || !transmitted_b1 ||
        !v34_b1_stream_complete(transmitted_b1) ||
        !v34_b1_receiver_complete(received_b1))
        return false;
    memset(channel, 0, sizeof(*channel));
    channel->tx_b1 = *transmitted_b1;
    channel->superframe_bits =
        (size_t)transmitted_b1->b1.geometry.bits_per_data_frame *
        transmitted_b1->b1.geometry.data_frames_per_superframe;
    if (channel->superframe_bits > V34_MAX_SUPERFRAME_BITS ||
        !v34_data_receiver_init_after_b1(&channel->rx, received_b1))
        return false;
    v34_uart_init(&channel->uart);
    return true;
}

size_t v34_data_channel_write(v34_data_channel *channel,
                              const uint8_t *bytes, size_t count)
{
    if (!channel || channel->failed)
        return 0;
    return v34_uart_write(&channel->uart, bytes, count);
}

size_t v34_data_channel_read(v34_data_channel *channel,
                             uint8_t *bytes, size_t capacity)
{
    if (!channel || channel->failed)
        return 0;
    return v34_uart_read(&channel->uart, bytes, capacity);
}

static bool prepare_transmit_superframe(v34_data_channel *channel)
{
    if (!v34_uart_fill_bits(&channel->uart, channel->tx_bits,
                            channel->superframe_bits))
        return false;
    if (!channel->tx_started) {
        if (!v34_data_stream_init_after_b1(
                &channel->tx, &channel->tx_b1,
                channel->tx_bits, channel->superframe_bits))
            return false;
        channel->tx_started = true;
        return true;
    }
    return v34_data_stream_next_superframe(
        &channel->tx, channel->tx_bits, channel->superframe_bits);
}

bool v34_data_channel_generate(v34_data_channel *channel,
                               uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    if (!channel || !pcma || channel->failed)
        return false;
    if (!channel->tx_started || v34_data_stream_complete(&channel->tx)) {
        if (!prepare_transmit_superframe(channel)) {
            channel->failed = true;
            return false;
        }
    }
    if (v34_data_stream_generate(&channel->tx, pcma,
                                 V34_PCMA_PACKET_SAMPLES) !=
        V34_PCMA_PACKET_SAMPLES) {
        channel->failed = true;
        return false;
    }
    channel->tx_packets++;
    return true;
}

bool v34_data_channel_receive(v34_data_channel *channel,
                              const uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    size_t count;

    if (!channel || !pcma || channel->failed)
        return false;
    if (v34_data_receiver_complete(&channel->rx) &&
        !v34_data_receiver_next_superframe(&channel->rx)) {
        channel->failed = true;
        return false;
    }
    if (!v34_data_receiver_feed(&channel->rx, pcma,
                                V34_PCMA_PACKET_SAMPLES)) {
        channel->failed = true;
        return false;
    }
    while ((count = v34_data_receiver_read(
                &channel->rx, channel->rx_bits,
                sizeof(channel->rx_bits))) != 0u) {
        if (!v34_uart_feed_bits(&channel->uart,
                                channel->rx_bits, count)) {
            channel->failed = true;
            return false;
        }
    }
    channel->rx_packets++;
    return true;
}
