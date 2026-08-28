#ifndef SOFTMODEM_V34_PHASE2_H
#define SOFTMODEM_V34_PHASE2_H

#include "v34_caps.h"
#include "v34_info.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    v34_symbol_rate symbol_rate;
    unsigned data_rate;
    bool high_carrier;
    uint8_t preemphasis;
} v34_phase2_selection;

bool v34_phase2_select(const v34_info1c *probe,
                       uint8_t allowed_symbols,
                       uint16_t allowed_rates,
                       unsigned maximum_rate,
                       v34_phase2_selection *selection);

#endif
