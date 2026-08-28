#include "ansam.h"
#include "pcma.h"
#include <math.h>
#include <stdio.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
int main(void){struct ansam_generator g;struct ansam_detector mod,plain;ansam_generator_init(&g);ansam_detector_init(&mod);ansam_detector_init(&plain);uint64_t at=0;for(int b=0;b<75;b++){int16_t x[160],y[160],z[160];uint8_t law[160];ansam_generate(&g,x,160);for(int i=0;i<160;i++)z[i]=(int16_t)(sin(2*M_PI*2100.0*(at+i)/8000.0)*9000);at+=160;pcma_encode_buffer(x,law,160);pcma_decode_buffer(law,y,160);ansam_detect(&mod,y,160);pcma_encode_buffer(z,law,160);pcma_decode_buffer(law,y,160);ansam_detect(&plain,y,160);}if(!ansam_present(&mod)||ansam_present(&plain)){fprintf(stderr,"ANSam detector failed: mod=%d plain=%d reversals=%u ratio=%.2f\n",ansam_present(&mod),ansam_present(&plain),mod.reversals,mod.max_amp/mod.min_amp);return 1;}puts("ANSam/PCMA test passed");return 0;}
