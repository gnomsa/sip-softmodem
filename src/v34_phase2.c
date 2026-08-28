#include "v34_phase2.h"

#include <stddef.h>
#include <math.h>
#include <string.h>

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

static bool candidate(const v34_info1c *probe, unsigned symbol,
                      uint8_t allowed_symbols, uint16_t allowed_rates,
                      unsigned maximum_rate, v34_phase2_selection *selection)
{
    const v34_probe_result *result = &probe->symbol[symbol];
    unsigned rate = (unsigned)result->projected_rate_2400 * V34_RATE_STEP;
    if ((allowed_symbols & V34_SYMBOL_BIT(symbol)) == 0 || rate == 0 ||
        rate > maximum_rate || !v34_rate_supported(allowed_rates, rate))
        return false;
    selection->symbol_rate = (v34_symbol_rate)symbol;
    selection->data_rate = rate;
    selection->high_carrier = result->high_carrier;
    selection->preemphasis = result->preemphasis;
    return true;
}

bool v34_phase2_select_duplex(const v34_info1c *answer_to_call_probe,
                              const v34_info1c *call_to_answer_probe,
                              uint8_t allowed_symbols,
                              uint16_t allowed_rates,
                              unsigned maximum_rate,
                              unsigned maximum_symbol_difference,
                              v34_phase2_duplex *duplex)
{
    unsigned a, c;
    unsigned best_score = 0, best_floor = 0;
    bool found = false;
    v34_phase2_duplex best = {0};
    if (answer_to_call_probe == NULL || call_to_answer_probe == NULL ||
        duplex == NULL || maximum_symbol_difference > 5u)
        return false;
    for (a = 0; a < V34_SYMBOL_COUNT; ++a) {
        v34_phase2_selection as;
        if (!candidate(answer_to_call_probe, a, allowed_symbols, allowed_rates,
                       maximum_rate, &as))
            continue;
        for (c = 0; c < V34_SYMBOL_COUNT; ++c) {
            v34_phase2_selection cs;
            unsigned difference = a > c ? a - c : c - a;
            unsigned score, floor;
            if (difference > maximum_symbol_difference ||
                !candidate(call_to_answer_probe, c, allowed_symbols, allowed_rates,
                           maximum_rate, &cs))
                continue;
            score = as.data_rate + cs.data_rate;
            floor = as.data_rate < cs.data_rate ? as.data_rate : cs.data_rate;
            if (!found || score > best_score ||
                (score == best_score && floor > best_floor)) {
                best.answer_to_call = as;
                best.call_to_answer = cs;
                best_score = score;
                best_floor = floor;
                found = true;
            }
        }
    }
    if (!found)
        return false;
    *duplex = best;
    return true;
}

bool v34_phase2_make_info1a(const v34_phase2_duplex *duplex,
                            uint8_t minimum_power_reduction,
                            uint8_t additional_power_reduction,
                            uint8_t md_length_35ms,
                            int16_t frequency_offset_002hz,
                            v34_info1a *info)
{
    if (duplex == NULL || info == NULL || minimum_power_reduction > 7u ||
        additional_power_reduction > 7u || md_length_35ms > 127u ||
        frequency_offset_002hz < -512 || frequency_offset_002hz > 511 ||
        duplex->call_to_answer.data_rate % V34_RATE_STEP != 0 ||
        duplex->call_to_answer.data_rate < V34_RATE_MIN ||
        duplex->call_to_answer.data_rate > V34_RATE_MAX)
        return false;
    info->minimum_power_reduction = minimum_power_reduction;
    info->additional_power_reduction = additional_power_reduction;
    info->md_length_35ms = md_length_35ms;
    info->high_carrier = duplex->call_to_answer.high_carrier;
    info->preemphasis = duplex->call_to_answer.preemphasis;
    info->projected_rate_2400 =
        (uint8_t)(duplex->call_to_answer.data_rate / V34_RATE_STEP);
    info->answer_symbol_rate = (uint8_t)duplex->answer_to_call.symbol_rate;
    info->call_symbol_rate = (uint8_t)duplex->call_to_answer.symbol_rate;
    info->frequency_offset_002hz = frequency_offset_002hz;
    return true;
}

static unsigned projected_rate(uint16_t rates, unsigned maximum,
                               unsigned penalty)
{
    unsigned rate = v34_highest_rate(rates & v34_rates_up_to(maximum));
    while (rate != 0u && penalty-- != 0u) {
        rates &= (uint16_t)~V34_RATE_BIT(rate / V34_RATE_STEP - 1u);
        rate = v34_highest_rate(rates & v34_rates_up_to(maximum));
    }
    return rate;
}

bool v34_phase2_make_info1c(const v34_probe_rx *probe,
                            uint8_t allowed_symbols,
                            uint16_t allowed_rates,
                            unsigned maximum_rate,
                            v34_info1c *info)
{
    static const unsigned symbol_ceiling[V34_SYMBOL_COUNT] = {
        21600u, 26400u, 26400u, 28800u, 31200u, 33600u
    };
    double average = 0.0, minimum = 0.0;
    double low = 0.0, high = 0.0, tilt_db;
    unsigned tone, symbol, penalty, preemphasis;

    if (probe == NULL || info == NULL || !v34_probe_rx_ready(probe))
        return false;
    for (tone = 0; tone < V34_PROBE_TONES; ++tone) {
        double amplitude = v34_probe_rx_amplitude(probe, tone);
        average += amplitude;
        if (tone == 0u || amplitude < minimum)
            minimum = amplitude;
        if (v34_probe_frequency[tone] <= 1650u)
            low += amplitude;
        else
            high += amplitude;
    }
    average /= V34_PROBE_TONES;
    low /= 9.0;
    high /= 12.0;
    if (average < 100.0 || low < 1.0 || high < 1.0)
        return false;

    if (minimum / average >= 0.75)
        penalty = 0u;
    else if (minimum / average >= 0.60)
        penalty = 1u;
    else if (minimum / average >= 0.45)
        penalty = 2u;
    else
        penalty = 3u;
    tilt_db = 20.0 * log10(low / high);
    preemphasis = tilt_db <= 0.0 ? 0u : (unsigned)ceil(tilt_db / 1.5);
    if (preemphasis > 10u)
        preemphasis = 10u;

    memset(info, 0, sizeof(*info));
    for (symbol = 0; symbol < V34_SYMBOL_COUNT; ++symbol) {
        unsigned ceiling, rate;
        if ((allowed_symbols & V34_SYMBOL_BIT(symbol)) == 0u)
            continue;
        ceiling = symbol_ceiling[symbol] < maximum_rate ?
                  symbol_ceiling[symbol] : maximum_rate;
        rate = projected_rate(allowed_rates, ceiling, penalty);
        info->symbol[symbol].projected_rate_2400 =
            (uint8_t)(rate / V34_RATE_STEP);
        info->symbol[symbol].preemphasis = (uint8_t)preemphasis;
        info->symbol[symbol].high_carrier = high > low * 1.10;
    }
    return true;
}
