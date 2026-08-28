#include "v34_phase4_receiver.h"

#include <string.h>

static bool feed_symbol(v34_phase4_receiver *receiver, uint8_t phase)
{
    unsigned symbol = receiver->symbols;
    uint8_t expected;

    if (symbol < 8u) {
        if (!v34_j_prime4_phase(&receiver->j_scrambler,
                                &receiver->j_index,
                                &receiver->j_rotation, &expected) ||
            phase != expected)
            return false;
    } else if (symbol < 520u) {
        if (symbol == 8u)
            v34_scrambler_init(&receiver->trn_scrambler,
                               receiver->sender_call_modem);
        if (!v34_trn4_phase(&receiver->trn_scrambler, &expected) ||
            phase != expected)
            return false;
        receiver->trn_rotation = ((12u - phase) % 12u) / 3u;
    } else if (symbol < 564u) {
        bool decoded;
        if (symbol == 520u &&
            !v34_mp_receiver_init(&receiver->mp_receiver,
                                  &receiver->trn_scrambler,
                                  receiver->trn_rotation))
            return false;
        decoded = v34_mp_receiver_feed(
            &receiver->mp_receiver, phase, &receiver->mp);
        if (symbol == 563u) {
            if (!decoded || receiver->mp.acknowledge)
                return false;
            receiver->have_mp = true;
        } else if (decoded) {
            return false;
        }
    } else if (symbol < 608u) {
        bool decoded;
        if (symbol == 564u)
            v34_mp_receiver_next(&receiver->mp_receiver);
        decoded = v34_mp_receiver_feed(
            &receiver->mp_receiver, phase, &receiver->mp_prime);
        if (symbol == 607u) {
            if (!decoded || !receiver->mp_prime.acknowledge)
                return false;
            receiver->have_mp_prime = true;
        } else if (decoded) {
            return false;
        }
    } else if (symbol < 618u) {
        if (symbol == 608u) {
            receiver->e_scrambler = receiver->mp_receiver.descrambler;
            receiver->e_rotation = receiver->mp_receiver.rotation;
            receiver->e_index = 0;
        }
        if (!v34_e4_phase(&receiver->e_scrambler,
                          &receiver->e_index,
                          &receiver->e_rotation, &expected) ||
            phase != expected)
            return false;
    } else {
        return false;
    }
    receiver->symbols++;
    if (receiver->symbols == 618u)
        receiver->complete = receiver->have_mp && receiver->have_mp_prime;
    return true;
}

bool v34_phase4_receiver_init(v34_phase4_receiver *receiver,
                              bool sender_call_modem,
                              const v34_scrambler *phase3_scrambler,
                              unsigned phase3_rotation,
                              v34_symbol_rate symbol_rate,
                              bool high_carrier,
                              unsigned sample_rate)
{
    if (!receiver || !phase3_scrambler || phase3_rotation > 3u)
        return false;
    memset(receiver, 0, sizeof(*receiver));
    receiver->sender_call_modem = sender_call_modem;
    receiver->j_scrambler = *phase3_scrambler;
    receiver->j_rotation = phase3_rotation;
    return v34_training_rx_init(&receiver->rx, symbol_rate,
                                high_carrier, sample_rate);
}

bool v34_phase4_receiver_feed(v34_phase4_receiver *receiver, uint8_t pcma)
{
    uint8_t phase;

    if (!receiver || receiver->failed)
        return false;
    if (receiver->complete)
        return true;
    if (!v34_training_rx_pcma(&receiver->rx, pcma, &phase, NULL))
        return true;
    if (!feed_symbol(receiver, phase)) {
        receiver->failed = true;
        return false;
    }
    return true;
}

bool v34_phase4_receiver_complete(const v34_phase4_receiver *receiver)
{
    return receiver && receiver->complete && !receiver->failed &&
           receiver->symbols == 618u;
}
