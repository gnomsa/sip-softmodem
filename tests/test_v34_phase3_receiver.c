#include "v34_phase3_receiver.h"
#include "v34_phase3_stream.h"

#include <assert.h>
#include <stdio.h>

static void run(v34_symbol_rate rate, v34_phase3_role role, uint8_t md)
{
    v34_phase3_stream tx;
    v34_phase3_receiver rx;
    uint8_t packet[160];
    size_t packets;

    assert(v34_phase3_stream_init(&tx, role, md, rate, true, 8000, 9000));
    assert(v34_phase3_receiver_init(&rx, role, md, rate, true, 8000));
    for (packets = 0; packets < 200; ++packets) {
        assert(v34_phase3_stream_generate(&tx, packet, sizeof(packet)) ==
               sizeof(packet));
        assert(v34_phase3_receiver_feed(&rx, packet, sizeof(packet)) ==
               sizeof(packet));
        assert(!v34_phase3_receiver_failed(&rx));
        if (v34_phase3_receiver_j_detected(&rx)) {
            assert(v34_phase3_stream_finish_j(&tx));
            assert(v34_phase3_receiver_finish_j(&rx));
            break;
        }
    }
    assert(packets < 200);
    assert(v34_phase3_stream_complete(&tx));
    assert(v34_phase3_receiver_complete(&rx));
    assert(tx.scrambler.history == rx.scrambler.history);
    assert(tx.scrambler.tap == rx.scrambler.tap);
    assert(tx.j_rotation == rx.trn_rotation);
    assert(tx.clock.phase == rx.rx.clock.phase);
}

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate) {
        run((v34_symbol_rate)rate, V34_PHASE3_CALL, 0);
        run((v34_symbol_rate)rate, V34_PHASE3_ANSWER, 0);
        run((v34_symbol_rate)rate, V34_PHASE3_CALL, 1);
        run((v34_symbol_rate)rate, V34_PHASE3_ANSWER, 1);
    }
    puts("v34 autonomous Phase 3 receiver: all plans and rates pass");
    return 0;
}
