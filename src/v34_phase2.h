#ifndef SOFTMODEM_V34_PHASE2_H
#define SOFTMODEM_V34_PHASE2_H

#include "v34_caps.h"
#include "v34_info.h"
#include "v34_probe.h"

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

typedef struct {
    v34_phase2_selection answer_to_call;
    v34_phase2_selection call_to_answer;
} v34_phase2_duplex;

bool v34_phase2_select_duplex(const v34_info1c *answer_to_call_probe,
                              const v34_info1c *call_to_answer_probe,
                              uint8_t allowed_symbols,
                              uint16_t allowed_rates,
                              unsigned maximum_rate,
                              unsigned maximum_symbol_difference,
                              v34_phase2_duplex *duplex);
bool v34_phase2_make_info1a(const v34_phase2_duplex *duplex,
                            uint8_t minimum_power_reduction,
                            uint8_t additional_power_reduction,
                            uint8_t md_length_35ms,
                            int16_t frequency_offset_002hz,
                            v34_info1a *info);
bool v34_phase2_make_info1c(const v34_probe_rx *probe,
                            uint8_t allowed_symbols,
                            uint16_t allowed_rates,
                            unsigned maximum_rate,
                            v34_info1c *info);

#endif
