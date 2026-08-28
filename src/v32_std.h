#ifndef SIP_SOFTMODEM_V32_STD_H
#define SIP_SOFTMODEM_V32_STD_H
#include <stdint.h>
enum v32_std_role { V32_STD_CALL, V32_STD_ANSWER };
struct v32_std_scrambler {uint32_t history;int tap;};
void v32_std_scrambler_init(struct v32_std_scrambler*s,enum v32_std_role role);
int v32_std_scramble(struct v32_std_scrambler*s,int input);
int v32_std_descramble(struct v32_std_scrambler*s,int input);
unsigned v32_std_diff_encode(unsigned input_dibit,unsigned previous_output);
uint16_t v32_std_rate_word(int rate4800,int rate9600,int trellis);
uint16_t v32_std_e_word(int rate,int trellis);
int v32_std_rate_decode(uint16_t word,int*rate4800,int*rate9600,int*trellis);
#endif
