#include "tone_detector.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void signal_block(int16_t*x,double hz,double amplitude,unsigned offset){
    for(unsigned i=0;i<160;i++)x[i]=(int16_t)(amplitude*sin(2*M_PI*hz*(offset+i)/8000.0));
}
int main(void){
    struct tone_detector ans,wrong;tone_detector_init(&ans,2100,160);tone_detector_init(&wrong,2100,160);
    int16_t x[160];
    for(unsigned b=0;b<3;b++){signal_block(x,2100,9000,b*160);tone_detector_process(&ans,x,160);}
    for(unsigned b=0;b<6;b++){signal_block(x,1800,9000,b*160);tone_detector_process(&wrong,x,160);}
    if(!tone_detector_present(&ans)||tone_detector_present(&wrong)){fprintf(stderr,"tone detector failed\n");return 1;}
    for(unsigned b=0;b<3;b++){for(unsigned i=0;i<160;i++)x[i]=0;tone_detector_process(&ans,x,160);}
    if(tone_detector_present(&ans)){fprintf(stderr,"tone release failed\n");return 1;}
    puts("tone detector tests passed");return 0;
}
