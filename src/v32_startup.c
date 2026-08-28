#include "v32_startup.h"

static void enter(struct v32_startup *s, enum v32_startup_phase p, unsigned n)
{
    s->phase = p;
    s->symbols = n;
}

void v32_startup_init(struct v32_startup *s, enum v32_std_role role,
                      int allow_4800, int allow_9600)
{
    *s = (struct v32_startup){0};
    s->role = role;
    s->allow_4800 = !!allow_4800;
    s->allow_9600 = !!allow_9600;
    s->local_rate_word = v32_std_rate_word(s->allow_4800, s->allow_9600, 0);
    enter(s, role == V32_STD_CALL ? V32_START_WAIT_ANSWER_TONE : V32_START_AC,
          role == V32_STD_CALL ? 0 : 128);
}

void v32_startup_answer_tone(struct v32_startup *s)
{
    if (s->role == V32_STD_CALL && s->phase == V32_START_WAIT_ANSWER_TONE)
        enter(s, V32_START_AA, 0); /* repeated until the first reversal */
}

void v32_startup_carrier(struct v32_startup *s)
{
    if (s->role == V32_STD_ANSWER && s->phase == V32_START_AC)
        enter(s, V32_START_CA, 0);
}

void v32_startup_phase_reversal(struct v32_startup *s)
{
    if (s->role == V32_STD_CALL && s->phase == V32_START_AA)
        enter(s, V32_START_CC, 64);
    else if (s->role == V32_STD_CALL && s->phase == V32_START_CC)
        enter(s, V32_START_WAIT_REMOTE_S, 0);
    else if (s->role == V32_STD_ANSWER && s->phase == V32_START_CA)
        enter(s, V32_START_AC, 64);
}

void v32_startup_amplitude_drop(struct v32_startup *s)
{
    if (s->role == V32_STD_ANSWER && s->phase == V32_START_AC)
        enter(s, V32_START_SILENCE_16, 16);
}

void v32_startup_remote_s(struct v32_startup *s)
{
    if (s->role == V32_STD_CALL && s->phase == V32_START_WAIT_REMOTE_S)
        enter(s, V32_START_TRAIN_1, 1552);
    else if (s->role == V32_STD_ANSWER && s->phase == V32_START_RATE_1)
        enter(s, V32_START_WAIT_REMOTE_TRAIN, 0);
}

void v32_startup_segment_done(struct v32_startup *s)
{
    switch (s->phase) {
    case V32_START_SILENCE_16: enter(s, V32_START_TRAIN_1, 1552); break;
    case V32_START_TRAIN_1: enter(s, V32_START_RATE_1, 0); break;
    case V32_START_WAIT_REMOTE_TRAIN: enter(s, V32_START_TRAIN_2, 1552); break;
    case V32_START_TRAIN_2: enter(s, V32_START_RATE_2, 0); break;
    case V32_START_E: enter(s, V32_START_ONES_128, 128); break;
    case V32_START_ONES_128: enter(s, V32_START_DATA, 0); break;
    default: break;
    }
}

int v32_startup_rate_word(struct v32_startup *s, uint16_t word)
{
    int r4800, r9600, trellis;
    if (s->phase != V32_START_RATE_1 && s->phase != V32_START_RATE_2)
        return 0;
    if (v32_std_rate_decode(word, &r4800, &r9600, &trellis) < 0) return 0;
    (void)trellis;
    if (word == s->remote_rate_word) s->identical_rate_words++;
    else { s->remote_rate_word = word; s->identical_rate_words = 1; }
    if (s->identical_rate_words < 2) return 0;
    s->selected_rate = s->allow_9600 && r9600 ? 9600 :
                       s->allow_4800 && r4800 ? 4800 : 0;
    if (!s->selected_rate) { enter(s, V32_START_FAILED, 0); return -1; }
    enter(s, V32_START_E, 16);
    return s->selected_rate;
}

void v32_startup_remote_e(struct v32_startup *s)
{
    if (s->phase == V32_START_E) v32_startup_segment_done(s);
}

const char *v32_startup_phase_name(enum v32_startup_phase p)
{
    static const char *const names[] = {
        "wait-answer-tone", "AA", "CC", "wait-S", "AC", "CA", "silence-16",
        "S/Sbar/TRN-1", "R1", "wait-remote-training", "S/Sbar/TRN-2", "R2/R3",
        "E", "ones-128", "data", "failed"
    };
    return p <= V32_START_FAILED ? names[p] : "unknown";
}
