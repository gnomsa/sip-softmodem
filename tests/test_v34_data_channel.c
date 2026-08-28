#include "v34_data_channel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const double scale = 180.0;
    v34_b1_stream b1;
    v34_b1_receiver b1_receiver;
    v34_data_channel channel;
    uint8_t input[2048];
    uint8_t output[2048];
    size_t output_count = 0;
    size_t i;
    unsigned packet;

    assert(v34_b1_stream_init(&b1, V34_SYMBOL_3429, 33600, true,
                              V34_TRELLIS_64, true, true,
                              8000, scale));
    assert(v34_b1_receiver_init(&b1_receiver, V34_SYMBOL_3429, 33600,
                                true, V34_TRELLIS_64, true, true,
                                8000, scale));
    while (!v34_b1_stream_complete(&b1)) {
        uint8_t sample;
        assert(v34_b1_stream_generate(&b1, &sample, 1u) == 1u);
        assert(v34_b1_receiver_feed(&b1_receiver, sample));
    }
    assert(v34_data_channel_init_after_b1(
        &channel, &b1, &b1_receiver));

    for (i = 0; i < sizeof(input); ++i)
        input[i] = (uint8_t)((i * 109u) ^ (i >> 3u));
    assert(v34_data_channel_write(&channel, input, 777u) == 777u);
    for (packet = 0; packet < 42u; ++packet) {
        uint8_t pcma[V34_PCMA_PACKET_SAMPLES];
        uint8_t chunk[37];
        size_t count;

        if (packet == 9u)
            assert(v34_data_channel_write(
                &channel, input + 777u, sizeof(input) - 777u) ==
                sizeof(input) - 777u);
        assert(v34_data_channel_generate(&channel, pcma));
        assert(v34_data_channel_receive(&channel, pcma));
        while ((count = v34_data_channel_read(
                    &channel, chunk, sizeof(chunk))) != 0u) {
            assert(output_count + count <= sizeof(output));
            memcpy(output + output_count, chunk, count);
            output_count += count;
        }
    }
    assert(output_count == sizeof(input));
    assert(memcmp(output, input, sizeof(input)) == 0);
    assert(channel.tx_packets == 42u);
    assert(channel.rx_packets == 42u);
    assert(channel.uart.framing_errors == 0u);
    assert(channel.rx.soft_corrections > 0u);
    printf("v34 byte channel: %zu bytes in 42 PCMA packets, %llu repairs\n",
           output_count,
           (unsigned long long)channel.rx.soft_corrections);
    return 0;
}
