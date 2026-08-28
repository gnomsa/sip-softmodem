#include "v34_phase3.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    v34_phase3_plan p;
    assert(v34_phase3_build_tx_plan(V34_PHASE3_ANSWER, 2, &p));
    assert(p.count == 9);
    assert(p.event[0].signal == V34_P3_SILENCE && p.event[0].milliseconds == 70);
    assert(p.event[1].signal == V34_P3_S && p.event[1].symbols == 128);
    assert(p.event[2].signal == V34_P3_S_BAR && p.event[2].symbols == 16);
    assert(p.event[3].signal == V34_P3_MD && p.event[3].milliseconds == 70);
    assert(p.event[6].signal == V34_P3_PP && p.event[6].symbols == 288);
    assert(p.event[7].signal == V34_P3_TRN && p.event[7].symbols == 512);
    assert(p.event[8].signal == V34_P3_J);
    assert(v34_phase3_build_tx_plan(V34_PHASE3_CALL, 0, &p));
    assert(p.count == 5 && p.event[0].signal == V34_P3_S);
    assert(p.event[2].signal == V34_P3_PP && p.event[2].symbols == 288);
    {
        v34_phase3_cursor cursor;
        const v34_phase3_event *event;
        assert(v34_phase3_cursor_init(&cursor, &p));
        event = v34_phase3_current(&cursor);
        assert(event != NULL && event->signal == V34_P3_S);
        assert(v34_phase3_advance(&cursor, 64));
        assert(cursor.index == 0 && cursor.elapsed == 64);
        assert(!v34_phase3_advance(&cursor, 65));
        assert(v34_phase3_advance(&cursor, 64));
        assert(v34_phase3_current(&cursor)->signal == V34_P3_S_BAR);
        assert(v34_phase3_advance(&cursor, 16));
        assert(v34_phase3_advance(&cursor, 288));
        assert(v34_phase3_advance(&cursor, 512));
        assert(v34_phase3_current(&cursor)->signal == V34_P3_J);
        assert(v34_phase3_advance(&cursor, 0));
        assert(cursor.complete && v34_phase3_current(&cursor) == NULL);
    }
    puts("v34 Phase 3 transmit plan tests: ok");
    return 0;
}
