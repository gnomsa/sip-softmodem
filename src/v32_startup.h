#ifndef SIP_SOFTMODEM_V32_STARTUP_H
#define SIP_SOFTMODEM_V32_STARTUP_H

#include "v32_std.h"
#include <stdint.h>

/* V.32 section 5.4 start-up controller.  DSP detectors report the events;
 * the controller decides which line segment must be sent next. */
enum v32_startup_phase {
    V32_START_WAIT_ANSWER_TONE,
    V32_START_AA,
    V32_START_CC,
    V32_START_WAIT_REMOTE_S,
    V32_START_AC,
    V32_START_CA,
    V32_START_SILENCE_16,
    V32_START_TRAIN_1,
    V32_START_RATE_1,
    V32_START_WAIT_REMOTE_TRAIN,
    V32_START_TRAIN_2,
    V32_START_RATE_2,
    V32_START_E,
    V32_START_ONES_128,
    V32_START_DATA,
    V32_START_FAILED
};

struct v32_startup {
    enum v32_std_role role;
    enum v32_startup_phase phase;
    unsigned symbols;
    int allow_4800;
    int allow_9600;
    int selected_rate;
    uint16_t local_rate_word;
    uint16_t remote_rate_word;
    unsigned identical_rate_words;
};

void v32_startup_init(struct v32_startup *s, enum v32_std_role role,
                      int allow_4800, int allow_9600);
void v32_startup_answer_tone(struct v32_startup *s);
void v32_startup_carrier(struct v32_startup *s);
void v32_startup_phase_reversal(struct v32_startup *s);
void v32_startup_amplitude_drop(struct v32_startup *s);
void v32_startup_remote_s(struct v32_startup *s);
void v32_startup_segment_done(struct v32_startup *s);
int v32_startup_rate_word(struct v32_startup *s, uint16_t word);
void v32_startup_remote_e(struct v32_startup *s);
const char *v32_startup_phase_name(enum v32_startup_phase phase);

#endif
