#include "v34_phase3_stream.h"
#include "pcma.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    v34_phase3_stream stream;
    uint8_t packet[160];
    size_t packets = 0;
    long long energy = 0;
    unsigned i;

    assert(v34_phase3_stream_init(&stream, V34_PHASE3_CALL, 0,
                                  V34_SYMBOL_3429, true, 8000, 9000));
    while (v34_phase3_current(&stream.cursor)->signal != V34_P3_J && packets < 100) {
        assert(v34_phase3_stream_generate(&stream, packet, sizeof(packet)) ==
               sizeof(packet));
        for (i = 0; i < sizeof(packet); ++i) {
            int sample = pcma_decode(packet[i]);
            energy += (long long)sample * sample;
        }
        packets++;
    }
    assert(!v34_phase3_stream_complete(&stream));
    assert(packets > 1 && packets < 100);
    assert(stream.clock.symbols >= 944);
    assert(stream.active_samples >=
           v34_samples_for_symbols(V34_SYMBOL_3429, 8000, 944));
    assert(v34_phase3_stream_generate(&stream, packet, sizeof(packet)) == sizeof(packet));
    assert(!v34_phase3_stream_complete(&stream));
    assert(stream.j_bit_index > 16u);
    assert(v34_phase3_stream_finish_j(&stream));
    assert(v34_phase3_stream_complete(&stream));
    assert(energy > 1000000000LL);
    puts("v34 Phase 3 160-byte PCMA stream test: ok");
    return 0;
}
