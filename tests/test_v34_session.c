#include "v34_session.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    v34_session caller;
    v34_session answerer;
    v34_session_config call_config = {
        true, 0, V34_SYMBOL_3429, 33600, V34_TRELLIS_64, true,
        8000, 9000.0, 180.0,
        {14, 12, false, 2, false, true, false, V34_RATE_ALL_MASK, true},
        false, V34_SYMBOL_2400, V34_SYMBOL_2400, false, false
    };
    v34_session_config answer_config = {
        false, 0, V34_SYMBOL_3429, 33600, V34_TRELLIS_64, true,
        8000, 9000.0, 180.0,
        {14, 13, false, 2, false, true, false, V34_RATE_ALL_MASK, true},
        false, V34_SYMBOL_2400, V34_SYMBOL_2400, false, false
    };
    uint8_t call_input[1403], answer_input[997];
    uint8_t call_output[1403], answer_output[997];
    size_t call_count = 0, answer_count = 0, i;
    uint8_t call_delay[10][160], answer_delay[10][160];
    unsigned packet;

    assert(v34_session_init(&caller, &call_config));
    assert(v34_session_init(&answerer, &answer_config));
    for (i = 0; i < sizeof(call_input); ++i)
        call_input[i] = (uint8_t)((i * 47u) ^ (i >> 1u));
    for (i = 0; i < sizeof(answer_input); ++i)
        answer_input[i] = (uint8_t)((i * 89u) ^ (i >> 3u));
    assert(v34_session_write(&caller, call_input, sizeof(call_input)) ==
           sizeof(call_input));
    assert(v34_session_write(&answerer, answer_input, sizeof(answer_input)) ==
           sizeof(answer_input));

    for (packet = 0; packet < 220u; ++packet) {
        uint8_t call_pcma[160], answer_pcma[160], bytes[53];
        size_t count;
        assert(v34_session_generate(&caller, call_pcma));
        assert(v34_session_generate(&answerer, answer_pcma));
        if (packet >= 10u) {
            assert(v34_session_receive(
                &caller, answer_delay[(packet - 10u) % 10u]));
            assert(v34_session_receive(
                &answerer, call_delay[(packet - 10u) % 10u]));
        }
        memcpy(call_delay[packet % 10u], call_pcma, sizeof(call_pcma));
        memcpy(answer_delay[packet % 10u], answer_pcma, sizeof(answer_pcma));
        if (packet < 10u)
            continue;
        while ((count = v34_session_read(
                    &answerer, bytes, sizeof(bytes))) != 0u) {
            assert(call_count + count <= sizeof(call_output));
            memcpy(call_output + call_count, bytes, count);
            call_count += count;
        }
        while ((count = v34_session_read(
                    &caller, bytes, sizeof(bytes))) != 0u) {
            assert(answer_count + count <= sizeof(answer_output));
            memcpy(answer_output + answer_count, bytes, count);
            answer_count += count;
        }
        if (call_count == sizeof(call_input) &&
            answer_count == sizeof(answer_input))
            break;
    }
    assert(packet < 220u);
    assert(v34_session_connected(&caller));
    assert(v34_session_connected(&answerer));
    assert(caller.rates.call_to_answer == 33600u);
    assert(caller.rates.answer_to_call == 28800u);
    assert(answerer.rates.call_to_answer == 33600u);
    assert(answerer.rates.answer_to_call == 28800u);
    assert(memcmp(call_output, call_input, sizeof(call_input)) == 0);
    assert(memcmp(answer_output, answer_input, sizeof(answer_input)) == 0);
    printf("v34 delayed session: Phase 3/4/B1, 33600/28800, %zu + %zu bytes\n",
           call_count, answer_count);
    return 0;
}
