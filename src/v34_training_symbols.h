#ifndef SOFTMODEM_V34_TRAINING_SYMBOLS_H
#define SOFTMODEM_V34_TRAINING_SYMBOLS_H

#include <stdbool.h>
#include <stdint.h>

#define V34_PP_PERIOD 48u
#define V34_PP_SYMBOLS 288u

/* Phase is expressed in multiples of pi/6, counterclockwise. */
bool v34_pp_phase(unsigned symbol, uint8_t *phase_pi_6);
bool v34_s_phase(unsigned symbol, bool reversed, uint8_t *phase_pi_6);

#endif
