#ifndef SIP_SOFTMODEM_V32_RETRAIN_H
#define SIP_SOFTMODEM_V32_RETRAIN_H
#include "v32_training.h"

enum v32_retrain_state { V32_RETRAIN_IDLE, V32_RETRAIN_REQUEST, V32_RETRAIN_ACK, V32_RETRAIN_RESTART };
struct v32_retrain { enum v32_retrain_state state; unsigned tx_count, ab_count, cd_count; };
void v32_retrain_init(struct v32_retrain *r);
void v32_retrain_request(struct v32_retrain *r);
enum v32_carrier_state v32_retrain_next(struct v32_retrain *r);
int v32_retrain_put(struct v32_retrain *r, enum v32_carrier_state state);
#endif
