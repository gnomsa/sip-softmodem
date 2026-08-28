#include "pcma.h"
#include "v32_line.h"
#include "v32_rate.h"
#include <stdio.h>
static int role(enum v32_std_role role,int max9600){uint16_t sent=v32_std_rate_word(1,max9600,0),got=0;struct v32_rate_tx enc;struct v32_rate_rx dec;v32_rate_tx_init(&enc,role,sent,V32_STATE_A);v32_rate_rx_init(&dec,role,V32_STATE_A);struct v32_line tx,rx;v32_line_init(&tx);v32_line_init(&rx);int found=0;for(int block=0;block<30&&!found;block++){enum v32_carrier_state states[48];for(int i=0;i<48;i++)states[i]=v32_rate_tx_next(&enc);v32_line_write(&tx,states,48);int16_t x[160],y[160];uint8_t law[160];v32_line_generate(&tx,x,160);pcma_encode_buffer(x,law,160);pcma_decode_buffer(law,y,160);v32_line_receive(&rx,y,160);size_t n=v32_line_read(&rx,states,48);for(size_t i=0;i<n;i++)if(v32_rate_rx_put(&dec,states[i],&got))found=1;}printf("V.32 %s rate word/PCMA: sent %04x got %04x\n",role==V32_STD_CALL?"GPC":"GPA",sent,got);return found&&got==sent?0:1;}
int main(void){return role(V32_STD_CALL,1)||role(V32_STD_ANSWER,0);}
