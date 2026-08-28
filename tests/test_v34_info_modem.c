#include "pcma.h"
#include "v34_info.h"
#include "v34_info_modem.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void exercise(v34_info_modem_role role, const uint8_t *frame,
                     size_t bit_count)
{
    v34_info_modem_tx tx;
    v34_info_modem_rx rx;
    uint8_t packet[197];
    uint8_t decoded[(V34_INFO_MODEM_MAX_BITS + 7u) / 8u] = {0};
    uint8_t silence[173];
    static const size_t chunks[] = {1, 159, 37, 197, 80, 13, 161};
    size_t chunk_at = 0u, received_bits = 0u, guard = 0u;

    memset(silence, pcma_encode(0), sizeof(silence));
    v34_info_modem_tx_init(&tx, role);
    v34_info_modem_rx_init(&rx, role, bit_count);
    assert(v34_info_modem_tx_start(&tx, frame, bit_count));

    /* RTP may deliver silence before the remote INFO carrier arrives. */
    v34_info_modem_rx_process(&rx, silence, sizeof(silence));
    while (!v34_info_modem_rx_ready(&rx) && guard++ < 100u) {
        size_t count = chunks[chunk_at++ %
                              (sizeof(chunks) / sizeof(chunks[0]))];
        v34_info_modem_tx_generate(&tx, packet, count);
        v34_info_modem_rx_process(&rx, packet, count);
    }
    assert(v34_info_modem_tx_done(&tx));
    assert(v34_info_modem_rx_ready(&rx));
    assert(v34_info_modem_rx_read(&rx, decoded, sizeof(decoded),
                                  &received_bits));
    assert(received_bits == bit_count);
    assert(memcmp(frame, decoded, (bit_count + 7u) / 8u) == 0);
}

int main(void)
{
    v34_info0 info0 = {
        true, true, true, true, true, true, true, true, true, 5,
        true, true, V34_CLOCK_EXTERNAL, false
    };
    v34_info1a info1a = {2, 1, 31, true, 7, 13, 5, 4, -91};
    v34_info1c info1c = {0};
    uint8_t frame0[V34_INFO0_BYTES];
    uint8_t frame1a[V34_INFO1A_BYTES];
    uint8_t frame1c[V34_INFO1C_BYTES];
    unsigned i;

    info1c.minimum_power_reduction = 3;
    info1c.additional_power_reduction = 2;
    info1c.md_length_35ms = 42;
    info1c.frequency_offset_002hz = 75;
    for (i = 0; i < 6u; ++i) {
        info1c.symbol[i].high_carrier = (i & 1u) != 0u;
        info1c.symbol[i].preemphasis = (uint8_t)(i + 1u);
        info1c.symbol[i].projected_rate_2400 = (uint8_t)(9u + i);
    }

    assert(v34_info0_encode(&info0, frame0));
    assert(v34_info1a_encode(&info1a, frame1a));
    assert(v34_info1c_encode(&info1c, frame1c));

    exercise(V34_INFO_CALL_MODEM, frame0, V34_INFO0_BITS);
    exercise(V34_INFO_ANSWER_MODEM, frame0, V34_INFO0_BITS);
    exercise(V34_INFO_ANSWER_MODEM, frame1a, V34_INFO1A_BITS);
    exercise(V34_INFO_CALL_MODEM, frame1c, V34_INFO1C_BITS);

    puts("v34 INFO DPSK modem tests: ok");
    return 0;
}
