#include "v34_modem_session.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PACKET 160u
#define DELAY 10u

static v34_modem_session_config config(v34_info_modem_role role)
{
    v34_modem_session_config c;
    memset(&c, 0, sizeof(c));
    c.role = role;
    c.info0.symbol_2743 = true;
    c.info0.symbol_2800 = true;
    c.info0.symbol_3429 = true;
    c.info0.carrier_3000_low = true;
    c.info0.carrier_3000_high = true;
    c.info0.carrier_3200_low = true;
    c.info0.carrier_3200_high = true;
    c.info0.allow_3429 = true;
    c.info0.maximum_symbol_rate_difference = 1u;
    c.info0.cme = true;
    c.info0.constellation_1664 = true;
    c.info0.clock_source = role == V34_INFO_CALL_MODEM ?
                           V34_CLOCK_INTERNAL : V34_CLOCK_RECEIVE;
    c.info0.acknowledge = role == V34_INFO_ANSWER_MODEM;
    c.allowed_symbols = V34_SYMBOL_ALL_MASK;
    c.allowed_rates = V34_RATE_ALL_MASK;
    c.maximum_rate = 33600u;
    c.maximum_symbol_difference = 1u;
    c.trellis = V34_TRELLIS_64;
    c.expanded_shaping = true;
    c.sample_rate = 8000u;
    c.training_amplitude = 9000.0;
    c.coordinate_scale = 180.0;
    return c;
}

int main(void)
{
    v34_modem_session caller, answer;
    v34_modem_session_config cc = config(V34_INFO_CALL_MODEM);
    v34_modem_session_config ac = config(V34_INFO_ANSWER_MODEM);
    uint8_t call_in[701], answer_in[503], call_out[701], answer_out[503];
    uint8_t cq[DELAY][PACKET], aq[DELAY][PACKET];
    size_t call_count = 0u, answer_count = 0u, i;
    unsigned packet, call_training_at = 999u, answer_training_at = 999u;

    assert(v34_modem_session_init(&caller, &cc));
    assert(v34_modem_session_init(&answer, &ac));
    for (i = 0; i < sizeof(call_in); ++i)
        call_in[i] = (uint8_t)(i * 37u + 11u);
    for (i = 0; i < sizeof(answer_in); ++i)
        answer_in[i] = (uint8_t)(i * 71u + 3u);
    assert(v34_modem_session_write(&caller, call_in, sizeof(call_in)) ==
           sizeof(call_in));
    assert(v34_modem_session_write(&answer, answer_in, sizeof(answer_in)) ==
           sizeof(answer_in));

    for (packet = 0; packet < 500u; ++packet) {
        unsigned slot = packet % DELAY;
        uint8_t bytes[61];
        size_t count;
        if (packet >= DELAY) {
            unsigned old = (packet - DELAY) % DELAY;
            bool cr = v34_modem_session_receive(&caller, aq[old]);
            bool ar = v34_modem_session_receive(&answer, cq[old]);
            if (!cr || !ar)
            {
                v34_mode cm = {0}, am = {0};
                bool cmok = v34_phase2_session_mode(&caller.phase2, &cm,
                                                     NULL, NULL);
                bool amok = v34_phase2_session_mode(&answer.phase2, &am,
                                                     NULL, NULL);
                fprintf(stderr, "receive failed packet %u result %d/%d "
                        "full %d/%d phase2 %d/%d training %d/%d "
                        "mode %d:%u/%u/%d/%d %d:%u/%u/%d/%d start %u/%u\n",
                        packet, cr, ar, caller.state, answer.state,
                        caller.phase2.state, answer.phase2.state,
                        caller.training.state, answer.training.state,
                        cmok, cm.tx_rate, cm.rx_rate, cm.tx_symbol, cm.rx_symbol,
                        amok, am.tx_rate, am.rx_rate, am.tx_symbol, am.rx_symbol,
                        call_training_at, answer_training_at);
            }
            assert(cr && ar);
        }
        assert(v34_modem_session_generate(&caller, cq[slot]));
        assert(v34_modem_session_generate(&answer, aq[slot]));
        if (caller.state == V34_MODEM_SESSION_TRAINING_DATA &&
            call_training_at == 999u)
            call_training_at = packet;
        if (answer.state == V34_MODEM_SESSION_TRAINING_DATA &&
            answer_training_at == 999u)
            answer_training_at = packet;
        while ((count = v34_modem_session_read(&answer, bytes,
                                                sizeof(bytes))) != 0u) {
            assert(call_count + count <= sizeof(call_out));
            memcpy(call_out + call_count, bytes, count);
            call_count += count;
        }
        while ((count = v34_modem_session_read(&caller, bytes,
                                                sizeof(bytes))) != 0u) {
            assert(answer_count + count <= sizeof(answer_out));
            memcpy(answer_out + answer_count, bytes, count);
            answer_count += count;
        }
        if (call_count == sizeof(call_in) && answer_count == sizeof(answer_in))
            break;
    }
    if (packet == 500u)
        fprintf(stderr, "full states %d/%d training %d/%d bytes %zu/%zu\n",
                caller.state, answer.state, caller.training.state,
                answer.training.state, call_count, answer_count);
    assert(packet < 500u);
    assert(v34_modem_session_connected(&caller));
    assert(v34_modem_session_connected(&answer));
    assert(memcmp(call_in, call_out, sizeof(call_in)) == 0);
    assert(memcmp(answer_in, answer_out, sizeof(answer_in)) == 0);
    {
        v34_mode mode;
        assert(v34_modem_session_mode(&caller, &mode));
        assert(mode.tx_rate == 33600u && mode.rx_rate == 33600u);
    }
    v34_modem_session_media_gap(&caller);
    assert(!v34_modem_session_connected(&caller));
    assert(!v34_modem_session_generate(&caller, cq[0]));
    printf("v34 full modem: Phase 2/3/4/B1/data, 33600/33600, "
           "%zu + %zu bytes in %u packets\n",
           call_count, answer_count, packet);
    return 0;
}
