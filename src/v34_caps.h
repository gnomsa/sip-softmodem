#ifndef SOFTMODEM_V34_CAPS_H
#define SOFTMODEM_V34_CAPS_H

#include <stdbool.h>
#include <stdint.h>

#define V34_RATE_MIN 2400u
#define V34_RATE_MAX 33600u
#define V34_RATE_STEP 2400u
#define V34_RATE_COUNT 14u
#define V34_RATE_BIT(index) ((uint16_t)(1u << (index)))
#define V34_RATE_MANDATORY_MASK ((uint16_t)0x0fffu)
#define V34_RATE_ALL_MASK ((uint16_t)0x3fffu)

typedef enum {
    V34_SYMBOL_2400 = 0,
    V34_SYMBOL_2743,
    V34_SYMBOL_2800,
    V34_SYMBOL_3000,
    V34_SYMBOL_3200,
    V34_SYMBOL_3429,
    V34_SYMBOL_COUNT
} v34_symbol_rate;

#define V34_SYMBOL_BIT(rate) ((uint8_t)(1u << (unsigned)(rate)))
#define V34_SYMBOL_MANDATORY_MASK ((uint8_t)(V34_SYMBOL_BIT(V34_SYMBOL_2400) | \
                                             V34_SYMBOL_BIT(V34_SYMBOL_3000) | \
                                             V34_SYMBOL_BIT(V34_SYMBOL_3200)))
#define V34_SYMBOL_ALL_MASK ((uint8_t)((1u << V34_SYMBOL_COUNT) - 1u))

typedef struct {
    unsigned nominal_baud;
    unsigned rate_num;
    unsigned rate_den;
    unsigned low_carrier_num;
    unsigned low_carrier_den;
    unsigned high_carrier_num;
    unsigned high_carrier_den;
    bool mandatory;
} v34_symbol_info;

typedef struct {
    uint16_t tx_rates;
    uint16_t rx_rates;
    uint8_t tx_symbols;
    uint8_t rx_symbols;
} v34_capabilities;

typedef struct {
    unsigned tx_rate;
    unsigned rx_rate;
    v34_symbol_rate tx_symbol;
    v34_symbol_rate rx_symbol;
} v34_mode;

uint16_t v34_rates_up_to(unsigned maximum);
unsigned v34_highest_rate(uint16_t mask);
bool v34_rate_supported(uint16_t mask, unsigned rate);
const v34_symbol_info *v34_get_symbol_info(v34_symbol_rate rate);
double v34_symbol_baud(v34_symbol_rate rate);
double v34_carrier_hz(v34_symbol_rate rate, bool high_carrier);
bool v34_negotiate(const v34_capabilities *local,
                   const v34_capabilities *remote,
                   unsigned maximum,
                   v34_mode *mode);

#endif
