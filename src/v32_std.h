#ifndef SIP_SOFTMODEM_V32_STD_H
#define SIP_SOFTMODEM_V32_STD_H
#include <stdint.h>
enum v32_std_role { V32_STD_CALL, V32_STD_ANSWER };
#define V32_RATE_4800  (1u<<0)
#define V32_RATE_7200  (1u<<1)
#define V32_RATE_9600  (1u<<2)
#define V32_RATE_12000 (1u<<3)
#define V32_RATE_14400 (1u<<4)
struct v32_std_scrambler {uint32_t history;int tap;};
void v32_std_scrambler_init(struct v32_std_scrambler*s,enum v32_std_role role);
int v32_std_scramble(struct v32_std_scrambler*s,int input);
int v32_std_descramble(struct v32_std_scrambler*s,int input);
unsigned v32_std_diff_encode(unsigned input_dibit,unsigned previous_output);
unsigned v32bis_trellis_diff_encode(unsigned input_dibit,unsigned previous_output);
int v32bis_trellis_diff_decode(unsigned output,unsigned previous_output);
uint16_t v32_std_rate_word(int rate4800,int rate9600,int trellis);
uint16_t v32_std_e_word(int rate,int trellis);
int v32_std_rate_decode(uint16_t word,int*rate4800,int*rate9600,int*trellis);
int v32_std_e_decode(uint16_t word,int*rate,int*trellis);
uint16_t v32bis_rate_word(unsigned rates,int trellis);
uint16_t v32bis_e_word(int rate,int trellis);
int v32bis_rate_decode(uint16_t word,unsigned*rates,int*trellis,int*bis);
int v32bis_e_decode(uint16_t word,int*rate,int*trellis,int*bis);
int v32_highest_rate(unsigned rates);
#endif
