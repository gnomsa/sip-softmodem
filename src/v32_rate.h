#ifndef SIP_SOFTMODEM_V32_RATE_H
#define SIP_SOFTMODEM_V32_RATE_H
#include "v32_training.h"
struct v32_rate_tx {struct v32_std_scrambler scr;uint16_t word;unsigned bit,previous;};
struct v32_rate_rx {struct v32_std_scrambler descr;uint16_t word,last;unsigned bits,previous,repeats;int detected;};
void v32_rate_tx_init(struct v32_rate_tx*t,enum v32_std_role role,uint16_t word,enum v32_carrier_state previous);
enum v32_carrier_state v32_rate_tx_next(struct v32_rate_tx*t);
void v32_rate_rx_init(struct v32_rate_rx*r,enum v32_std_role remote_role,enum v32_carrier_state previous);
int v32_rate_rx_put(struct v32_rate_rx*r,enum v32_carrier_state state,uint16_t*word);
#endif
