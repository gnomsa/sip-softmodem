#include "v32bis_data.h"
#include "v32bis_map.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void run(int rate){struct v32bis_data a,b;assert(v32bis_data_init(&a,V32_STD_CALL,rate)==0);assert(v32bis_data_init(&b,V32_STD_ANSWER,rate)==0);uint8_t source[256],got[300]={0};for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*37u+11u);assert(v32bis_data_write(&a,source,sizeof source)==sizeof source);size_t have=0;for(unsigned n=0;n<3000&&have<sizeof source;n++){unsigned label=v32bis_data_next(&a);struct v32bis_point p;assert(v32bis_map_point(rate,label,&p)==0);double noise=(int)(n%7)-3;assert(v32bis_data_put(&b,p.i+noise*.03,p.q-noise*.02)>=0);have+=v32bis_data_read(&b,got+have,sizeof got-have);}assert(have>=sizeof source&&!memcmp(source,got,sizeof source));}
int main(void){run(7200);run(9600);run(12000);run(14400);struct v32bis_data d;assert(v32bis_data_init(&d,V32_STD_CALL,4800)<0);puts("V.32bis data codec: exact bytes at 7200/9600/12000/14400 pass");return 0;}
