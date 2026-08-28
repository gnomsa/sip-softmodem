#include "pcma.h"
#include "v8.h"
#include "v8_fsk.h"
#include <stdio.h>
int main(void){
    struct v8_menu menu={.modes=V8_MODE_V21|V8_MODE_V22|V8_MODE_V32};uint8_t sent[128],received[256];size_t n=v8_encode_menu(&menu,sent,sizeof sent);
    struct v8_fsk tx,rx;v8_fsk_init(&tx,0);v8_fsk_init(&rx,0);v8_fsk_set_sequence(&tx,sent,n);size_t got=0;
    for(int block=0;block<200&&got<n*2;block++){int16_t pcm[160],decoded[160];uint8_t law[160];v8_fsk_generate(&tx,pcm,160);pcma_encode_buffer(pcm,law,160);pcma_decode_buffer(law,decoded,160);v8_fsk_receive(&rx,decoded,160);got+=v8_fsk_read(&rx,received+got,sizeof received-got);}
    if(got<n*2){fprintf(stderr,"short V.8 FSK receive\n");return 1;}
    /* Fixed 8 kHz test clocks may have one leading decision; find two exact repeated menus. */
    for(size_t off=0;off+n*2<=got;off++){size_t i=0;for(;i<n*2;i++)if(received[off+i]!=sent[i%n])break;if(i==n*2){puts("V.8 V.21(L)/PCMA test passed");return 0;}}
    fprintf(stderr,"V.8 FSK sequence mismatch\n");return 1;
}
