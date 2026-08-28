#include "v34_caps.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

static void test_rates(void)
{
    unsigned rate;

    assert(v34_rates_up_to(2399) == 0);
    assert(v34_rates_up_to(2400) == V34_RATE_BIT(0));
    assert(v34_rates_up_to(28800) == V34_RATE_MANDATORY_MASK);
    assert(v34_rates_up_to(33600) == V34_RATE_ALL_MASK);
    assert(v34_rates_up_to(56000) == V34_RATE_ALL_MASK);
    assert(v34_highest_rate(V34_RATE_ALL_MASK) == 33600);
    assert(v34_highest_rate(V34_RATE_MANDATORY_MASK) == 28800);
    assert(v34_highest_rate(0) == 0);

    for (rate = V34_RATE_MIN; rate <= V34_RATE_MAX; rate += V34_RATE_STEP)
        assert(v34_rate_supported(V34_RATE_ALL_MASK, rate));
    assert(!v34_rate_supported(V34_RATE_ALL_MASK, 0));
    assert(!v34_rate_supported(V34_RATE_ALL_MASK, 9601));
    assert(!v34_rate_supported(V34_RATE_ALL_MASK, 36000));
}

static void test_symbols(void)
{
    static const unsigned baud[V34_SYMBOL_COUNT] = {2400, 2743, 2800, 3000, 3200, 3429};
    static const unsigned num[V34_SYMBOL_COUNT] = {1, 8, 7, 5, 4, 10};
    static const unsigned den[V34_SYMBOL_COUNT] = {1, 7, 6, 4, 3, 7};
    static const unsigned low_num[V34_SYMBOL_COUNT] = {2, 3, 3, 3, 4, 4};
    static const unsigned low_den[V34_SYMBOL_COUNT] = {3, 5, 5, 5, 7, 7};
    static const unsigned high_num[V34_SYMBOL_COUNT] = {3, 2, 2, 2, 3, 4};
    static const unsigned high_den[V34_SYMBOL_COUNT] = {4, 3, 3, 3, 5, 7};
    unsigned i;

    assert(V34_SYMBOL_MANDATORY_MASK ==
           (V34_SYMBOL_BIT(V34_SYMBOL_2400) |
            V34_SYMBOL_BIT(V34_SYMBOL_3000) |
            V34_SYMBOL_BIT(V34_SYMBOL_3200)));
    for (i = 0; i < V34_SYMBOL_COUNT; ++i) {
        const v34_symbol_info *info = v34_get_symbol_info((v34_symbol_rate)i);
        assert(info != NULL);
        assert(info->nominal_baud == baud[i]);
        assert(info->rate_num == num[i]);
        assert(info->rate_den == den[i]);
        assert(info->low_carrier_num == low_num[i]);
        assert(info->low_carrier_den == low_den[i]);
        assert(info->high_carrier_num == high_num[i]);
        assert(info->high_carrier_den == high_den[i]);
        assert(info->mandatory == ((V34_SYMBOL_MANDATORY_MASK & V34_SYMBOL_BIT(i)) != 0));
    }
    assert(v34_get_symbol_info(V34_SYMBOL_COUNT) == NULL);
    assert(fabs(v34_symbol_baud(V34_SYMBOL_2743) - 19200.0 / 7.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_2400, false) - 1600.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_2400, true) - 1800.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_3000, false) - 1800.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_3000, true) - 2000.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_3429, false) - 96000.0 / 49.0) < 1e-9);
    assert(fabs(v34_carrier_hz(V34_SYMBOL_3429, true) - 96000.0 / 49.0) < 1e-9);
}

static void test_asymmetric_negotiation(void)
{
    const v34_capabilities local = {
        .tx_rates = V34_RATE_ALL_MASK,
        .rx_rates = V34_RATE_MANDATORY_MASK,
        .tx_symbols = V34_SYMBOL_ALL_MASK,
        .rx_symbols = V34_SYMBOL_MANDATORY_MASK,
    };
    const v34_capabilities remote = {
        .tx_rates = v34_rates_up_to(14400),
        .rx_rates = V34_RATE_ALL_MASK,
        .tx_symbols = V34_SYMBOL_BIT(V34_SYMBOL_3000),
        .rx_symbols = V34_SYMBOL_BIT(V34_SYMBOL_3429),
    };
    v34_mode mode;

    assert(v34_negotiate(&local, &remote, 33600, &mode));
    assert(mode.tx_rate == 33600);
    assert(mode.rx_rate == 14400);
    assert(mode.tx_symbol == V34_SYMBOL_3429);
    assert(mode.rx_symbol == V34_SYMBOL_3000);

    assert(v34_negotiate(&local, &remote, 9600, &mode));
    assert(mode.tx_rate == 9600);
    assert(mode.rx_rate == 9600);

    {
        v34_capabilities incompatible = remote;
        incompatible.rx_symbols = V34_SYMBOL_BIT(V34_SYMBOL_2743);
        incompatible.rx_rates = 0;
        assert(!v34_negotiate(&local, &incompatible, 33600, &mode));
    }
}

int main(void)
{
    test_rates();
    test_symbols();
    test_asymmetric_negotiation();
    puts("v34 capability tests: ok");
    return 0;
}
