#include "v32bis_qam.h"
#include "v32bis_map.h"
#include "pcma.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void run(int rate){struct v32bis_qam tx,rx;assert(v32bis_qam_init(&tx,rate)==0&&v32bis_qam_init(&rx,rate)==0);unsigned count=rate==7200?16:rate==9600?32:rate==12000?64:128;uint8_t labels[480];for(unsigned n=0;n<480;n++)labels[n]=(uint8_t)((n*29u+7u)%count);assert(v32bis_qam_write(&tx,labels,480)==480);unsigned got=0;for(unsigned block=0;block<100&&got<480;block++){int16_t pcm[160],decoded[160];uint8_t alaw[160];v32bis_qam_generate(&tx,pcm,160);pcma_encode_buffer(pcm,alaw,160);pcma_decode_buffer(alaw,decoded,160);v32bis_qam_receive(&rx,decoded,160);struct v32bis_sample points[64];size_t n=v32bis_qam_read(&rx,points,64);for(size_t k=0;k<n&&got<480;k++){unsigned label;double d;assert(v32bis_map_nearest(rate,points[k].i,points[k].q,&label,&d)==0);assert(label==labels[got++]);}}assert(got==480);}

static void fractional_output(void)
{
    struct v32bis_qam tx,rx;
    struct v32bis_sample symbols[512],fractional[1024];
    size_t symbol_count=0,fractional_count=0;
    uint8_t labels[480];
    assert(v32bis_qam_init(&tx,9600)==0);
    assert(v32bis_qam_init(&rx,9600)==0);
    v32bis_qam_set_pulse_shaped(&tx,1);
    v32bis_qam_set_pulse_shaped(&rx,1);
    v32bis_qam_enable_fractional_output(&rx,1);
    for(unsigned n=0;n<480;n++)labels[n]=(uint8_t)((n*29u+7u)%32u);
    assert(v32bis_qam_write(&tx,labels,480)==480);
    for(unsigned block=0;block<12;block++){
        int16_t pcm[160],decoded[160];uint8_t alaw[160];
        v32bis_qam_generate(&tx,pcm,160);
        pcma_encode_buffer(pcm,alaw,160);
        pcma_decode_buffer(alaw,decoded,160);
        v32bis_qam_receive(&rx,decoded,160);
        symbol_count+=v32bis_qam_read(&rx,symbols+symbol_count,
                                      512-symbol_count);
        fractional_count+=v32bis_qam_read_fractional(
            &rx,fractional+fractional_count,1024-fractional_count);
    }
    assert(symbol_count>400);
    assert(fractional_count==symbol_count*2);
    for(size_t n=0;n<symbol_count;n++){
        assert(fabs(fractional[2*n].i-symbols[n].i)<1e-12);
        assert(fabs(fractional[2*n].q-symbols[n].q)<1e-12);
    }
}

int main(void){run(7200);run(9600);run(12000);run(14400);fractional_output();struct v32bis_qam q;assert(v32bis_qam_init(&q,4800)<0);puts("V.32bis QAM/PCMA: exact symbols and two-sample fractional output pass");return 0;}
