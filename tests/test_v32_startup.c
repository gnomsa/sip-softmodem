#include "v32_startup.h"
#include <assert.h>
#include <stdio.h>

static void caller_selects_9600(void)
{
    struct v32_startup s;
    uint16_t both = v32_std_rate_word(1, 1, 0);
    v32_startup_init(&s, V32_STD_CALL, 1, 1);
    assert(s.phase == V32_START_WAIT_ANSWER_TONE);
    v32_startup_answer_tone(&s); assert(s.phase == V32_START_AA);
    v32_startup_phase_reversal(&s); assert(s.phase == V32_START_CC && s.symbols == 64);
    v32_startup_phase_reversal(&s); assert(s.phase == V32_START_WAIT_REMOTE_S);
    v32_startup_remote_s(&s); assert(s.phase == V32_START_TRAIN_1 && s.symbols == 1552);
    v32_startup_segment_done(&s); assert(s.phase == V32_START_RATE_1);
    assert(v32_startup_rate_word(&s, both) == 0);
    assert(v32_startup_rate_word(&s, both) == 9600);
    assert(s.phase == V32_START_E);
    v32_startup_remote_e(&s); assert(s.phase == V32_START_ONES_128 && s.symbols == 128);
    v32_startup_segment_done(&s); assert(s.phase == V32_START_DATA);
}

static void answer_falls_back_4800(void)
{
    struct v32_startup s;
    uint16_t remote = v32_std_rate_word(1, 0, 0);
    v32_startup_init(&s, V32_STD_ANSWER, 1, 1);
    assert(s.phase == V32_START_AC && s.symbols == 128);
    v32_startup_carrier(&s); assert(s.phase == V32_START_CA);
    v32_startup_phase_reversal(&s); assert(s.phase == V32_START_AC && s.symbols == 64);
    v32_startup_amplitude_drop(&s); assert(s.phase == V32_START_SILENCE_16);
    v32_startup_segment_done(&s); assert(s.phase == V32_START_TRAIN_1);
    v32_startup_segment_done(&s); assert(s.phase == V32_START_RATE_1);
    assert(v32_startup_rate_word(&s, remote) == 0);
    assert(v32_startup_rate_word(&s, remote) == 4800);
}

static void no_common_rate_fails(void)
{
    struct v32_startup s;
    uint16_t only9600 = v32_std_rate_word(0, 1, 0);
    v32_startup_init(&s, V32_STD_CALL, 1, 0);
    s.phase = V32_START_RATE_1;
    assert(v32_startup_rate_word(&s, only9600) == 0);
    assert(v32_startup_rate_word(&s, only9600) == -1);
    assert(s.phase == V32_START_FAILED);
}

int main(void)
{
    caller_selects_9600();
    answer_falls_back_4800();
    no_common_rate_fails();
    puts("V.32 start-up: 9600 selection, 4800 fallback and no-common-rate pass");
    return 0;
}
