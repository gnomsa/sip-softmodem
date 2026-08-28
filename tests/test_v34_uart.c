#include "v34_uart.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    v34_uart tx, rx;
    uint8_t input[513], output[513], bits[5173];
    size_t generated = 0, consumed = 0, i;

    v34_uart_init(&tx);
    v34_uart_init(&rx);
    for (i = 0; i < sizeof(input); ++i)
        input[i] = (uint8_t)((i * 73u) ^ (i >> 1u));
    assert(v34_uart_write(&tx, input, sizeof(input)) == sizeof(input));
    assert(v34_uart_tx_pending(&tx) == sizeof(input));
    while (generated < sizeof(bits)) {
        size_t chunk = 17u;
        if (chunk > sizeof(bits) - generated)
            chunk = sizeof(bits) - generated;
        assert(v34_uart_fill_bits(&tx, bits + generated, chunk));
        generated += chunk;
    }
    while (consumed < generated) {
        size_t chunk = 23u;
        if (chunk > generated - consumed)
            chunk = generated - consumed;
        assert(v34_uart_feed_bits(&rx, bits + consumed, chunk));
        consumed += chunk;
    }
    assert(v34_uart_tx_pending(&tx) == 0u);
    assert(v34_uart_read(&rx, output, sizeof(output)) == sizeof(output));
    assert(memcmp(input, output, sizeof(input)) == 0);
    assert(rx.framing_errors == 0u);
    assert(bits[sizeof(input) * 10u] == 1u);

    memset(bits, 0, 10u);
    assert(v34_uart_feed_bits(&rx, bits, 10u));
    assert(rx.framing_errors == 1u);
    assert(v34_uart_read(&rx, output, sizeof(output)) == 0u);
    puts("v34 UART adapter: arbitrary chunks, mark idle and framing pass");
    return 0;
}
