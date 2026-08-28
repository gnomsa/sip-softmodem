#include "tone_detector.h"
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void tone_detector_init(struct tone_detector *d,double frequency,size_t block_samples){
    memset(d,0,sizeof *d);d->frequency=frequency;d->block_samples=block_samples?block_samples:160;
}

static void finish_block(struct tone_detector*d){
    double w=2.0*M_PI*d->frequency/8000.0;
    double power=d->s1*d->s1+d->s2*d->s2-2.0*cos(w)*d->s1*d->s2;
    /* A coherent tone must carry useful energy and dominate the block. */
    int hit=d->total>1.0e7 && power*d->block_samples>d->total*20.0;
    if(hit){d->present_blocks++;d->absent_blocks=0;}else{d->absent_blocks++;d->present_blocks=0;}
    if(d->present_blocks>=3)d->present=1;       /* >= 60 ms at 20 ms/block */
    if(d->absent_blocks>=3)d->present=0;
    d->s1=d->s2=d->total=0;d->samples=0;
}

void tone_detector_process(struct tone_detector*d,const int16_t*x,size_t n){
    double coeff=2.0*cos(2.0*M_PI*d->frequency/8000.0);
    for(size_t i=0;i<n;i++){
        double sample=x[i],s=sample+coeff*d->s1-d->s2;
        d->s2=d->s1;d->s1=s;d->total+=sample*sample;
        if(++d->samples==d->block_samples)finish_block(d);
    }
}
int tone_detector_present(const struct tone_detector*d){return d->present;}
