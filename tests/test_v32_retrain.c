#include "v32_retrain.h"
#include <assert.h>
#include <stdio.h>
int main(void){struct v32_retrain a,b;v32_retrain_init(&a);v32_retrain_init(&b);v32_retrain_request(&a);for(int i=0;i<256;i++)v32_retrain_put(&b,v32_retrain_next(&a));assert(b.state==V32_RETRAIN_ACK);for(int i=0;i<16;i++)v32_retrain_put(&a,v32_retrain_next(&b));assert(a.state==V32_RETRAIN_RESTART);assert(b.state==V32_RETRAIN_RESTART);puts("V.32 retrain S/Sbar request and acknowledgement pass");return 0;}
