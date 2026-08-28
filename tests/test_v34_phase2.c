#include "v34_phase2.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    v34_info1c probe = {0};
    v34_phase2_selection selection;

    probe.symbol[V34_SYMBOL_2400].projected_rate_2400 = 8;
    probe.symbol[V34_SYMBOL_2400].preemphasis = 1;
    probe.symbol[V34_SYMBOL_3000].projected_rate_2400 = 12;
    probe.symbol[V34_SYMBOL_3000].preemphasis = 5;
    probe.symbol[V34_SYMBOL_3200].projected_rate_2400 = 12;
    probe.symbol[V34_SYMBOL_3200].preemphasis = 7;
    probe.symbol[V34_SYMBOL_3200].high_carrier = true;
    probe.symbol[V34_SYMBOL_3429].projected_rate_2400 = 14;
    probe.symbol[V34_SYMBOL_3429].preemphasis = 9;

    assert(v34_phase2_select(&probe, V34_SYMBOL_ALL_MASK,
                             V34_RATE_ALL_MASK, 33600, &selection));
    assert(selection.symbol_rate == V34_SYMBOL_3429);
    assert(selection.data_rate == 33600 && selection.preemphasis == 9);

    assert(v34_phase2_select(&probe, V34_SYMBOL_MANDATORY_MASK,
                             V34_RATE_MANDATORY_MASK, 28800, &selection));
    assert(selection.symbol_rate == V34_SYMBOL_3200);
    assert(selection.data_rate == 28800);
    assert(selection.high_carrier && selection.preemphasis == 7);

    assert(v34_phase2_select(&probe, V34_SYMBOL_ALL_MASK,
                             V34_RATE_ALL_MASK, 19200, &selection));
    assert(selection.symbol_rate == V34_SYMBOL_2400);
    assert(selection.data_rate == 19200);

    assert(!v34_phase2_select(&probe, V34_SYMBOL_BIT(V34_SYMBOL_2800),
                              V34_RATE_ALL_MASK, 33600, &selection));

    {
        v34_info1c reverse = probe;
        v34_phase2_duplex duplex;
        v34_info1a info;
        reverse.symbol[V34_SYMBOL_3429].projected_rate_2400 = 12;
        reverse.symbol[V34_SYMBOL_3200].projected_rate_2400 = 14;
        assert(v34_phase2_select_duplex(&probe, &reverse,
                                        V34_SYMBOL_ALL_MASK,
                                        V34_RATE_ALL_MASK, 33600, 1, &duplex));
        assert(duplex.answer_to_call.symbol_rate == V34_SYMBOL_3429);
        assert(duplex.answer_to_call.data_rate == 33600);
        assert(duplex.call_to_answer.symbol_rate == V34_SYMBOL_3200);
        assert(duplex.call_to_answer.data_rate == 33600);
        assert(v34_phase2_make_info1a(&duplex, 2, 1, 40, -25, &info));
        assert(info.answer_symbol_rate == V34_SYMBOL_3429);
        assert(info.call_symbol_rate == V34_SYMBOL_3200);
        assert(info.projected_rate_2400 == 14);
        assert(info.frequency_offset_002hz == -25);
        assert(!v34_phase2_select_duplex(&probe, &reverse,
                                         V34_SYMBOL_ALL_MASK,
                                         V34_RATE_ALL_MASK, 33600, 6, &duplex));
    }
    puts("v34 Phase 2 mode selection tests: ok");
    return 0;
}
