#include "v34_caps.h"

#include <stddef.h>

static const v34_symbol_info symbol_table[V34_SYMBOL_COUNT] = {
    [V34_SYMBOL_2400] = {2400, 1, 1, 2, 3, 3, 4, true},
    [V34_SYMBOL_2743] = {2743, 8, 7, 3, 5, 2, 3, false},
    [V34_SYMBOL_2800] = {2800, 7, 6, 3, 5, 2, 3, false},
    [V34_SYMBOL_3000] = {3000, 5, 4, 3, 5, 2, 3, true},
    [V34_SYMBOL_3200] = {3200, 4, 3, 4, 7, 3, 5, true},
    [V34_SYMBOL_3429] = {3429, 10, 7, 4, 7, 4, 7, false},
};

uint16_t v34_rates_up_to(unsigned maximum)
{
    unsigned count;

    if (maximum < V34_RATE_MIN)
        return 0;
    if (maximum >= V34_RATE_MAX)
        return V34_RATE_ALL_MASK;
    count = maximum / V34_RATE_STEP;
    return (uint16_t)((1u << count) - 1u);
}

unsigned v34_highest_rate(uint16_t mask)
{
    int i;

    mask &= V34_RATE_ALL_MASK;
    for (i = (int)V34_RATE_COUNT - 1; i >= 0; --i) {
        if (mask & V34_RATE_BIT((unsigned)i))
            return ((unsigned)i + 1u) * V34_RATE_STEP;
    }
    return 0;
}

bool v34_rate_supported(uint16_t mask, unsigned rate)
{
    unsigned index;

    if (rate < V34_RATE_MIN || rate > V34_RATE_MAX || rate % V34_RATE_STEP != 0)
        return false;
    index = rate / V34_RATE_STEP - 1u;
    return (mask & V34_RATE_BIT(index)) != 0;
}

const v34_symbol_info *v34_get_symbol_info(v34_symbol_rate rate)
{
    if ((unsigned)rate >= V34_SYMBOL_COUNT)
        return NULL;
    return &symbol_table[rate];
}

static bool select_symbol(uint8_t mask, v34_symbol_rate *selected)
{
    int i;

    mask &= V34_SYMBOL_ALL_MASK;
    for (i = (int)V34_SYMBOL_COUNT - 1; i >= 0; --i) {
        if (mask & V34_SYMBOL_BIT((unsigned)i)) {
            *selected = (v34_symbol_rate)i;
            return true;
        }
    }
    return false;
}

bool v34_negotiate(const v34_capabilities *local,
                   const v34_capabilities *remote,
                   unsigned maximum,
                   v34_mode *mode)
{
    uint16_t maximum_mask;
    uint16_t tx_rates;
    uint16_t rx_rates;
    uint8_t tx_symbols;
    uint8_t rx_symbols;

    if (local == NULL || remote == NULL || mode == NULL)
        return false;

    maximum_mask = v34_rates_up_to(maximum);
    tx_rates = local->tx_rates & remote->rx_rates & maximum_mask;
    rx_rates = local->rx_rates & remote->tx_rates & maximum_mask;
    tx_symbols = local->tx_symbols & remote->rx_symbols;
    rx_symbols = local->rx_symbols & remote->tx_symbols;

    mode->tx_rate = v34_highest_rate(tx_rates);
    mode->rx_rate = v34_highest_rate(rx_rates);
    if (mode->tx_rate == 0 || mode->rx_rate == 0)
        return false;
    if (!select_symbol(tx_symbols, &mode->tx_symbol) ||
        !select_symbol(rx_symbols, &mode->rx_symbol))
        return false;
    return true;
}
