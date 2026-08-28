#include "v34_b1_data_link.h"

#include "pcma.h"

#include <string.h>

#define PENDING_MASK (V34_UART_QUEUE_SIZE - 1u)

bool v34_b1_data_link_init(v34_b1_data_link *link,
                           bool call_modem,
                           v34_symbol_rate tx_symbol_rate,
                           unsigned tx_data_rate,
                           v34_symbol_rate rx_symbol_rate,
                           unsigned rx_data_rate,
                           v34_trellis_kind trellis,
                           bool expanded_shaping,
                           unsigned sample_rate,
                           double coordinate_scale)
{
    if (!link)
        return false;
    memset(link, 0, sizeof(*link));
    return v34_b1_stream_init(
               &link->tx_b1, tx_symbol_rate, tx_data_rate,
               call_modem, trellis, expanded_shaping,
               call_modem, sample_rate, coordinate_scale) &&
           v34_b1_receiver_init(
               &link->rx_b1, rx_symbol_rate, rx_data_rate,
               !call_modem, trellis, expanded_shaping,
               !call_modem, sample_rate, coordinate_scale);
}

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
    double coordinate_scale)
{
    if (!transmitted_phase4 || !transmitted_phase4->complete ||
        transmitted_phase4->symbols != 618u || !received_phase4 ||
        !received_phase4->complete || received_phase4->failed ||
        received_phase4->symbols != 618u ||
        !v34_b1_data_link_init(
            link, call_modem, tx_symbol_rate, tx_data_rate,
            rx_symbol_rate, rx_data_rate, trellis, expanded_shaping,
            sample_rate, coordinate_scale))
        return false;

    /* E ends on a symbol boundary.  Carry both oscillators and the rational
     * symbol clocks across that boundary instead of reacquiring B1. */
    link->tx_b1.clock = transmitted_phase4->clock;
    link->tx_b1.tx.carrier_phase = transmitted_phase4->tx.carrier_phase;
    link->tx_b1.tx.carrier_step = transmitted_phase4->tx.carrier_step;
    link->rx_b1.rx = received_phase4->rx;
    return true;
}

static bool start_data(v34_b1_data_link *link)
{
    if (link->data_ready)
        return true;
    if (!v34_b1_stream_complete(&link->tx_b1) ||
        !v34_b1_receiver_complete(&link->rx_b1))
        return true;
    if (!v34_data_channel_init_after_b1(
            &link->data, &link->tx_b1, &link->rx_b1))
        return false;
    while (link->pending_head != link->pending_tail) {
        size_t end = link->pending_tail > link->pending_head ?
                     link->pending_tail : V34_UART_QUEUE_SIZE;
        size_t written = v34_data_channel_write(
            &link->data, link->pending + link->pending_head,
            end - link->pending_head);
        link->pending_head = (link->pending_head + written) & PENDING_MASK;
        if (written == 0u)
            return false;
    }
    link->data_ready = true;
    return true;
}

size_t v34_b1_data_link_write(v34_b1_data_link *link,
                              const uint8_t *bytes, size_t count)
{
    size_t written = 0;

    if (!link || (!bytes && count != 0u) || link->failed)
        return 0;
    if (link->data_ready)
        return v34_data_channel_write(&link->data, bytes, count);
    while (written < count) {
        size_t next = (link->pending_tail + 1u) & PENDING_MASK;
        if (next == link->pending_head)
            break;
        link->pending[link->pending_tail] = bytes[written++];
        link->pending_tail = next;
    }
    return written;
}

size_t v34_b1_data_link_read(v34_b1_data_link *link,
                             uint8_t *bytes, size_t capacity)
{
    if (!link || !link->data_ready || link->failed)
        return 0;
    return v34_data_channel_read(&link->data, bytes, capacity);
}

bool v34_b1_data_link_generate(
    v34_b1_data_link *link,
    uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    if (!link || !pcma || link->failed)
        return false;
    if (!start_data(link)) {
        link->failed = true;
        return false;
    }
    if (link->data_ready) {
        if (!v34_data_channel_generate(&link->data, pcma)) {
            link->failed = true;
            return false;
        }
    } else if (!v34_b1_stream_complete(&link->tx_b1)) {
        if (v34_b1_stream_generate(
                &link->tx_b1, pcma, V34_PCMA_PACKET_SAMPLES) !=
            V34_PCMA_PACKET_SAMPLES) {
            link->failed = true;
            return false;
        }
    } else {
        memset(pcma, pcma_encode(0), V34_PCMA_PACKET_SAMPLES);
    }
    link->packets++;
    return true;
}

bool v34_b1_data_link_receive(
    v34_b1_data_link *link,
    const uint8_t pcma[V34_PCMA_PACKET_SAMPLES])
{
    size_t sample;

    if (!link || !pcma || link->failed)
        return false;
    if (link->data_ready) {
        if (!link->rx_data_started) {
            uint8_t silence = pcma_encode(0);
            for (sample = 0; sample < V34_PCMA_PACKET_SAMPLES; ++sample)
                if (pcma[sample] != silence)
                    break;
            if (sample == V34_PCMA_PACKET_SAMPLES)
                return true;
            link->rx_data_started = true;
        }
        if (!v34_data_channel_receive(&link->data, pcma)) {
            link->failed = true;
            return false;
        }
        return true;
    }
    if (!link->rx_b1_started) {
        uint8_t silence = pcma_encode(0);
        for (sample = 0; sample < V34_PCMA_PACKET_SAMPLES; ++sample)
            if (pcma[sample] != silence)
                break;
        if (sample == V34_PCMA_PACKET_SAMPLES)
            return start_data(link);
        link->rx_b1_started = true;
    }
    for (sample = 0; sample < V34_PCMA_PACKET_SAMPLES; ++sample)
        if (!v34_b1_receiver_feed(&link->rx_b1, pcma[sample])) {
            link->failed = true;
            return false;
        }
    if (!start_data(link)) {
        link->failed = true;
        return false;
    }
    return true;
}

bool v34_b1_data_link_connected(const v34_b1_data_link *link)
{
    return link && link->data_ready && !link->failed;
}
