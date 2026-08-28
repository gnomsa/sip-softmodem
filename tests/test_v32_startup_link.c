#include "pcma.h"
#include "v32_line.h"
#include "v32_rate.h"
#include "v32_startup.h"
#include "v32_training.h"
#include <assert.h>
#include <stdio.h>

struct end {
    enum v32_std_role role;
    struct v32_line tx, rx;
    struct v32_training training;
    struct v32_rate_tx rate_tx;
    struct v32_rate_rx rate_rx;
    struct v32_startup startup;
    enum v32_carrier_state last;
};

static void send_block(struct v32_line *tx, struct v32_line *rx)
{
    int16_t pcm[160], decoded[160];
    uint8_t alaw[160];
    v32_line_generate(tx, pcm, 160);
    pcma_encode_buffer(pcm, alaw, 160);
    pcma_decode_buffer(alaw, decoded, 160);
    v32_line_receive(rx, decoded, 160);
}

static void put_training(struct end *e)
{
    enum v32_carrier_state states[1552];
    for (size_t i = 0; i < 1552; i++) states[i] = v32_training_next(&e->training);
    e->last = states[1551];
    assert(v32_line_write(&e->tx, states, 1552) == 1552);
}

static void drain_training(struct end *e)
{
    enum v32_carrier_state states[1600];
    size_t total = 0;
    while (total < 1552) {
        size_t n = v32_line_read(&e->rx, states + total, 1600 - total);
        if (!n) break;
        total += n;
    }
    assert(total >= 1552);
}

static void queue_rate(struct end *e, size_t count)
{
    enum v32_carrier_state states[64];
    while (count) {
        size_t n = count > 64 ? 64 : count;
        for (size_t i = 0; i < n; i++) states[i] = v32_rate_tx_next(&e->rate_tx);
        assert(v32_line_write(&e->tx, states, n) == n);
        count -= n;
    }
}

static void consume_rate(struct end *e)
{
    enum v32_carrier_state states[128];
    size_t n;
    while ((n = v32_line_read(&e->rx, states, 128)) != 0) {
        for (size_t i = 0; i < n; i++) {
            uint16_t word;
            if (v32_rate_rx_put(&e->rate_rx, states[i], &word))
                (void)v32_startup_rate_word(&e->startup, word);
        }
    }
}

int main(void)
{
    struct end call = {.role = V32_STD_CALL}, answer = {.role = V32_STD_ANSWER};
    v32_line_init(&call.tx); v32_line_init(&call.rx);
    v32_line_init(&answer.tx); v32_line_init(&answer.rx);
    v32_training_init(&call.training, call.role);
    v32_training_init(&answer.training, answer.role);
    v32_startup_init(&call.startup, call.role, 1, 1);
    v32_startup_init(&answer.startup, answer.role, 1, 0);
    call.startup.phase = answer.startup.phase = V32_START_RATE_1;

    put_training(&call); put_training(&answer);
    for (int i = 0; i < 34; i++) {
        send_block(&call.tx, &answer.rx);
        send_block(&answer.tx, &call.rx);
    }
    drain_training(&call); drain_training(&answer);

    v32_rate_tx_init(&call.rate_tx, call.role, call.startup.local_rate_word, call.last);
    v32_rate_tx_init(&answer.rate_tx, answer.role, answer.startup.local_rate_word, answer.last);
    v32_rate_rx_init(&call.rate_rx, answer.role, answer.last);
    v32_rate_rx_init(&answer.rate_rx, call.role, call.last);
    queue_rate(&call, 128); queue_rate(&answer, 128);
    for (int i = 0; i < 4; i++) {
        send_block(&call.tx, &answer.rx);
        send_block(&answer.tx, &call.rx);
        consume_rate(&call); consume_rate(&answer);
    }
    assert(call.startup.selected_rate == 4800);
    assert(answer.startup.selected_rate == 4800);
    printf("V.32 full training/rate PCMA link: caller 9600+4800, answer 4800 -> %d bit/s\n",
           call.startup.selected_rate);
    return 0;
}
