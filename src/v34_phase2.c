#include "v34_phase2.h"

#include <stddef.h>

bool v34_phase2_select(const v34_info1c *probe,
                       uint8_t allowed_symbols,
                       uint16_t allowed_rates,
                       unsigned maximum_rate,
                       v34_phase2_selection *selection)
{
    unsigned i;
    bool found = false;
    v34_phase2_selection best = {0};

    if (probe == NULL || selection == NULL)
        return false;
    allowed_symbols &= V34_SYMBOL_ALL_MASK;
    allowed_rates &= v34_rates_up_to(maximum_rate);

    for (i = 0; i < V34_SYMBOL_COUNT; ++i) {
        const v34_probe_result *result = &probe->symbol[i];
        unsigned rate = (unsigned)result->projected_rate_2400 * V34_RATE_STEP;

        if ((allowed_symbols & V34_SYMBOL_BIT(i)) == 0 || rate == 0 ||
            !v34_rate_supported(allowed_rates, rate))
            continue;
        if (!found || rate > best.data_rate ||
            (rate == best.data_rate && i > (unsigned)best.symbol_rate)) {
            best.symbol_rate = (v34_symbol_rate)i;
            best.data_rate = rate;
            best.high_carrier = result->high_carrier;
            best.preemphasis = result->preemphasis;
            found = true;
        }
    }
    if (!found)
        return false;
    *selection = best;
    return true;
}
