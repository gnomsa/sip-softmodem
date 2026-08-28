#ifndef SOFTMODEM_V34_UART_H
#define SOFTMODEM_V34_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V34_UART_QUEUE_SIZE 8192u

typedef struct {
    uint8_t tx_queue[V34_UART_QUEUE_SIZE];
    uint8_t rx_queue[V34_UART_QUEUE_SIZE];
    size_t tx_head, tx_tail, rx_head, rx_tail;
    unsigned tx_frame, tx_frame_bits;
    unsigned rx_frame, rx_data_bits;
    unsigned framing_errors;
    bool rx_receiving;
} v34_uart;

void v34_uart_init(v34_uart *uart);
size_t v34_uart_write(v34_uart *uart, const uint8_t *bytes, size_t count);
size_t v34_uart_read(v34_uart *uart, uint8_t *bytes, size_t capacity);
bool v34_uart_fill_bits(v34_uart *uart, uint8_t *bits, size_t count);
bool v34_uart_feed_bits(v34_uart *uart, const uint8_t *bits, size_t count);
size_t v34_uart_tx_pending(const v34_uart *uart);

#endif
