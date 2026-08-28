#include "v34_uart.h"

#include <string.h>

#define QUEUE_MASK (V34_UART_QUEUE_SIZE - 1u)

void v34_uart_init(v34_uart *uart)
{
    if (uart)
        memset(uart, 0, sizeof(*uart));
}

size_t v34_uart_write(v34_uart *uart, const uint8_t *bytes, size_t count)
{
    size_t written = 0;
    if (!uart || (!bytes && count != 0u))
        return 0;
    while (written < count) {
        size_t next = (uart->tx_tail + 1u) & QUEUE_MASK;
        if (next == uart->tx_head)
            break;
        uart->tx_queue[uart->tx_tail] = bytes[written++];
        uart->tx_tail = next;
    }
    return written;
}

size_t v34_uart_read(v34_uart *uart, uint8_t *bytes, size_t capacity)
{
    size_t read = 0;
    if (!uart || (!bytes && capacity != 0u))
        return 0;
    while (read < capacity && uart->rx_head != uart->rx_tail) {
        bytes[read++] = uart->rx_queue[uart->rx_head];
        uart->rx_head = (uart->rx_head + 1u) & QUEUE_MASK;
    }
    return read;
}

static uint8_t next_tx_bit(v34_uart *uart)
{
    uint8_t bit;
    if (uart->tx_frame_bits == 0u) {
        uint8_t byte;
        if (uart->tx_head == uart->tx_tail)
            return 1u;
        byte = uart->tx_queue[uart->tx_head];
        uart->tx_head = (uart->tx_head + 1u) & QUEUE_MASK;
        /* Start, eight data bits LSB first, and stop. */
        uart->tx_frame = (1u << 9u) | ((unsigned)byte << 1u);
        uart->tx_frame_bits = 10u;
    }
    bit = (uint8_t)(uart->tx_frame & 1u);
    uart->tx_frame >>= 1u;
    uart->tx_frame_bits--;
    return bit;
}

bool v34_uart_fill_bits(v34_uart *uart, uint8_t *bits, size_t count)
{
    size_t bit;
    if (!uart || (!bits && count != 0u))
        return false;
    for (bit = 0; bit < count; ++bit)
        bits[bit] = next_tx_bit(uart);
    return true;
}

static void put_rx_bit(v34_uart *uart, uint8_t bit)
{
    if (!uart->rx_receiving) {
        if (bit == 0u) {
            uart->rx_receiving = true;
            uart->rx_frame = 0;
            uart->rx_data_bits = 0;
        }
        return;
    }
    if (uart->rx_data_bits < 8u) {
        uart->rx_frame |= (unsigned)bit << uart->rx_data_bits;
        uart->rx_data_bits++;
        return;
    }
    if (bit != 0u) {
        size_t next = (uart->rx_tail + 1u) & QUEUE_MASK;
        if (next != uart->rx_head) {
            uart->rx_queue[uart->rx_tail] = (uint8_t)uart->rx_frame;
            uart->rx_tail = next;
        }
    } else {
        uart->framing_errors++;
    }
    uart->rx_receiving = false;
}

bool v34_uart_feed_bits(v34_uart *uart, const uint8_t *bits, size_t count)
{
    size_t bit;
    if (!uart || (!bits && count != 0u))
        return false;
    for (bit = 0; bit < count; ++bit) {
        if (bits[bit] > 1u)
            return false;
        put_rx_bit(uart, bits[bit]);
    }
    return true;
}

size_t v34_uart_tx_pending(const v34_uart *uart)
{
    if (!uart)
        return 0;
    return (uart->tx_tail - uart->tx_head) & QUEUE_MASK;
}
