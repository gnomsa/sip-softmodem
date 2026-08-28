#include "v34_b1_data_link.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    v34_b1_data_link caller;
    v34_b1_data_link answerer;
    uint8_t call_input[1537], answer_input[911];
    uint8_t call_output[1537], answer_output[911];
    size_t call_count = 0, answer_count = 0, i;
    unsigned packet;

    assert(v34_b1_data_link_init(
        &caller, true, V34_SYMBOL_3429, 33600,
        V34_SYMBOL_3429, 33600, V34_TRELLIS_64, true, 8000, 180.0));
    assert(v34_b1_data_link_init(
        &answerer, false, V34_SYMBOL_3429, 33600,
        V34_SYMBOL_3429, 33600, V34_TRELLIS_64, true, 8000, 180.0));
    for (i = 0; i < sizeof(call_input); ++i)
        call_input[i] = (uint8_t)((i * 29u) ^ (i >> 2u));
    for (i = 0; i < sizeof(answer_input); ++i)
        answer_input[i] = (uint8_t)((i * 131u) ^ (i >> 4u));
    assert(v34_b1_data_link_write(
        &caller, call_input, sizeof(call_input)) == sizeof(call_input));
    assert(v34_b1_data_link_write(
        &answerer, answer_input, sizeof(answer_input)) ==
        sizeof(answer_input));

    for (packet = 0; packet < 44u; ++packet) {
        uint8_t call_pcma[V34_PCMA_PACKET_SAMPLES];
        uint8_t answer_pcma[V34_PCMA_PACKET_SAMPLES];
        uint8_t chunk[41];
        size_t count;

        assert(v34_b1_data_link_generate(&caller, call_pcma));
        assert(v34_b1_data_link_generate(&answerer, answer_pcma));
        assert(v34_b1_data_link_receive(&caller, answer_pcma));
        assert(v34_b1_data_link_receive(&answerer, call_pcma));
        while ((count = v34_b1_data_link_read(
                    &answerer, chunk, sizeof(chunk))) != 0u) {
            assert(call_count + count <= sizeof(call_output));
            memcpy(call_output + call_count, chunk, count);
            call_count += count;
        }
        while ((count = v34_b1_data_link_read(
                    &caller, chunk, sizeof(chunk))) != 0u) {
            assert(answer_count + count <= sizeof(answer_output));
            memcpy(answer_output + answer_count, chunk, count);
            answer_count += count;
        }
        if (packet == 0u) {
            assert(!v34_b1_data_link_connected(&caller));
            assert(!v34_b1_data_link_connected(&answerer));
        }
        if (packet == 1u) {
            assert(v34_b1_data_link_connected(&caller));
            assert(v34_b1_data_link_connected(&answerer));
        }
    }
    assert(call_count == sizeof(call_input));
    assert(answer_count == sizeof(answer_input));
    assert(memcmp(call_output, call_input, sizeof(call_input)) == 0);
    assert(memcmp(answer_output, answer_input, sizeof(answer_input)) == 0);
    assert(caller.data.uart.framing_errors == 0u);
    assert(answerer.data.uart.framing_errors == 0u);
    printf("v34 duplex B1/data link: %zu + %zu bytes in 44 packets\n",
           call_count, answer_count);
    return 0;
}
