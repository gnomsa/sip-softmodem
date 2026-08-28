#ifndef SOFTMODEM_V34_TRAINING_SYMBOLS_H
#define SOFTMODEM_V34_TRAINING_SYMBOLS_H

#include <stdbool.h>
#include <stdint.h>

#define V34_PP_PERIOD 48u
#define V34_PP_SYMBOLS 288u

typedef struct {
    uint32_t history;
    unsigned tap;
} v34_scrambler;

/* Phase is expressed in multiples of pi/6, counterclockwise. */
bool v34_pp_phase(unsigned symbol, uint8_t *phase_pi_6);
bool v34_s_phase(unsigned symbol, bool reversed, uint8_t *phase_pi_6);
void v34_scrambler_init(v34_scrambler *scrambler, bool call_modem);
unsigned v34_scramble_bit(v34_scrambler *scrambler, unsigned input);
bool v34_trn4_phase(v34_scrambler *scrambler, uint8_t *phase_pi_6);
bool v34_j4_phase(v34_scrambler *scrambler, unsigned *bit_index,
                  unsigned *previous_rotation, uint8_t *phase_pi_6);

#endif
