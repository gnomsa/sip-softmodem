#include "v34_j_detector.h"
#include "v34_phase3_stream.h"
#include "v34_training_rx.h"

#include <assert.h>
#include <stdio.h>

static void run(v34_symbol_rate rate)
{
    v34_phase3_stream tx;
    v34_training_rx rx;
    v34_j_detector detector;
    v34_scrambler expected_scrambler;
    uint8_t packet[160], phase;
    unsigned i, trn_rotation = 0;
    uint64_t received_symbols = 0;
    bool detector_ready = false, detected = false;
    size_t packets;

    v34_scrambler_init(&expected_scrambler, true);
    for (i = 0; i < 512; ++i) {
        assert(v34_trn4_phase(&expected_scrambler, &phase));
        trn_rotation = ((12u - phase) % 12u) / 3u;
    }
    assert(v34_phase3_stream_init(&tx, V34_PHASE3_CALL, 0, rate,
                                  true, 8000, 9000));
    assert(v34_training_rx_init(&rx, rate, true, 8000));

    for (packets = 0; packets < 100 && !detected; ++packets) {
        assert(v34_phase3_stream_generate(&tx, packet, sizeof(packet)) ==
               sizeof(packet));
        for (i = 0; i < sizeof(packet); ++i) {
            if (!v34_training_rx_pcma(&rx, packet[i], &phase, NULL))
                continue;
            received_symbols++;
            if (received_symbols == 944u) {
                assert(v34_j_detector_init(&detector, &expected_scrambler,
                                           trn_rotation));
                detector_ready = true;
            } else if (received_symbols > 944u && detector_ready &&
                       v34_j_detector_feed(&detector, phase)) {
                detected = true;
                break;
            }
        }
    }
    assert(detected);
    assert(v34_phase3_stream_finish_j(&tx));
    assert(v34_phase3_stream_complete(&tx));
}

int main(void)
{
    unsigned rate;
    for (rate = 0; rate < V34_SYMBOL_COUNT; ++rate)
        run((v34_symbol_rate)rate);
    puts("v34 Phase 3 PCMA/J link: all six symbol rates pass");
    return 0;
}
