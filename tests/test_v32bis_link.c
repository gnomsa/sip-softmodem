#include "v32bis_data.h"
#include "v32bis_qam.h"
#include "pcma.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void run(int rate){struct v32bis_data txd,rxd;struct v32bis_qam txq,rxq;assert(v32bis_data_init(&txd,V32_STD_CALL,rate)==0&&v32bis_data_init(&rxd,V32_STD_ANSWER,rate)==0);assert(v32bis_qam_init(&txq,rate)==0&&v32bis_qam_init(&rxq,rate)==0);uint8_t source[512],got[600]={0};for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*73u+19u);assert(v32bis_data_write(&txd,source,sizeof source)==sizeof source);size_t have=0;unsigned blocks;for(blocks=0;blocks<500&&have<sizeof source;blocks++){uint8_t labels[48];for(unsigned n=0;n<48;n++)labels[n]=(uint8_t)v32bis_data_next(&txd);assert(v32bis_qam_write(&txq,labels,48)==48);int16_t pcm[160],decoded[160];uint8_t alaw[160];v32bis_qam_generate(&txq,pcm,160);pcma_encode_buffer(pcm,alaw,160);pcma_decode_buffer(alaw,decoded,160);v32bis_qam_receive(&rxq,decoded,160);struct v32bis_sample points[64];size_t n=v32bis_qam_read(&rxq,points,64);for(size_t k=0;k<n;k++)assert(v32bis_data_put(&rxd,points[k].i,points[k].q)>=0);have+=v32bis_data_read(&rxd,got+have,sizeof got-have);}assert(have>=sizeof source&&!memcmp(source,got,sizeof source));printf("V.32bis %d/PCMA: 512 exact bytes, %.1f B/s\n",rate,sizeof source/(blocks*.02));}
int main(void){run(7200);run(9600);run(12000);run(14400);return 0;}
