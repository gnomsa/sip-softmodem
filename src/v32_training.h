#ifndef SIP_SOFTMODEM_V32_TRAINING_H
#define SIP_SOFTMODEM_V32_TRAINING_H
#include "v32_std.h"
#include <stdint.h>
enum v32_training_segment { V32_SEG_S,V32_SEG_SBAR,V32_SEG_TRN,V32_SEG_RATE };
enum v32_carrier_state { V32_STATE_A,V32_STATE_B,V32_STATE_C,V32_STATE_D };
struct v32_training {enum v32_training_segment segment;unsigned index;struct v32_std_scrambler scrambler;};
void v32_training_init(struct v32_training*t,enum v32_std_role role);
enum v32_carrier_state v32_training_next(struct v32_training*t);
#endif
