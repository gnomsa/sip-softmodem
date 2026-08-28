#include "pcma.h"
#include "v32_line.h"
#include <stdio.h>
#define N (256+16+1280)
static int role(enum v32_std_role role){enum v32_carrier_state sent[N],got[N];struct v32_training t;v32_training_init(&t,role);for(int i=0;i<N;i++)sent[i]=v32_training_next(&t);struct v32_line tx,rx;v32_line_init(&tx);v32_line_init(&rx);v32_line_write(&tx,sent,N);size_t have=0;for(int b=0;b<400&&have<N;b++){int16_t x[160],y[160];uint8_t law[160];v32_line_generate(&tx,x,160);pcma_encode_buffer(x,law,160);pcma_decode_buffer(law,y,160);v32_line_receive(&rx,y,160);have+=v32_line_read(&rx,got+have,N-have);}size_t errors=0;for(size_t i=0;i<have;i++)errors+=got[i]!=sent[i];printf("V.32 %s S/Sbar/TRN over PCMA: %zu/%d states, %zu errors\n",role==V32_STD_CALL?"GPC":"GPA",have,N,errors);return have==N&&errors==0?0:1;}
int main(void){return role(V32_STD_CALL)||role(V32_STD_ANSWER);}
