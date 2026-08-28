#include "v32_retrain.h"

void v32_retrain_init(struct v32_retrain *r) { *r=(struct v32_retrain){0}; }
void v32_retrain_request(struct v32_retrain *r)
{ if(r->state==V32_RETRAIN_IDLE)r->state=V32_RETRAIN_REQUEST; }

enum v32_carrier_state v32_retrain_next(struct v32_retrain *r)
{
    if(r->state==V32_RETRAIN_REQUEST)return (r->tx_count++&1)?V32_STATE_B:V32_STATE_A;
    if(r->state==V32_RETRAIN_ACK){
        enum v32_carrier_state s=(r->tx_count++&1)?V32_STATE_D:V32_STATE_C;
        if(r->tx_count>=16)r->state=V32_RETRAIN_RESTART;
        return s;
    }
    return V32_STATE_A;
}

int v32_retrain_put(struct v32_retrain *r,enum v32_carrier_state s)
{
    if(r->state==V32_RETRAIN_IDLE){
        enum v32_carrier_state want=(r->ab_count&1)?V32_STATE_B:V32_STATE_A;
        r->ab_count=s==want?r->ab_count+1:(s==V32_STATE_A?1:0);
        if(r->ab_count>=256){r->state=V32_RETRAIN_ACK;r->tx_count=0;return 1;}
    }else if(r->state==V32_RETRAIN_REQUEST){
        enum v32_carrier_state want=(r->cd_count&1)?V32_STATE_D:V32_STATE_C;
        r->cd_count=s==want?r->cd_count+1:(s==V32_STATE_C?1:0);
        if(r->cd_count>=16){r->state=V32_RETRAIN_RESTART;return 1;}
    }
    return 0;
}
