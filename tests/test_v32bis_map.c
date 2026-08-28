#include "v32bis_map.h"
#include <assert.h>
#include <stdio.h>
int main(void){unsigned energy=0;for(unsigned label=0;label<128;label++){struct v32bis_point p;assert(v32bis_map_point(14400,label,&p)==0);energy+=(unsigned)(p.i*p.i+p.q*p.q);for(unsigned other=0;other<label;other++){struct v32bis_point q;assert(v32bis_map_point(14400,other,&q)==0);assert(p.i!=q.i||p.q!=q.q);}unsigned got=999;double d=-1;assert(v32bis_map_nearest(14400,p.i+.1,p.q-.1,&got,&d)==0&&got==label&&d<.021);}struct v32bis_point p;assert(v32bis_map_point(14400,0,&p)==0&&p.i==-8&&p.q==-1);assert(v32bis_map_point(14400,127,&p)==0&&p.i==-5&&p.q==2);assert(energy==5760);assert(v32bis_map_point(12000,0,&p)<0);puts("V.32bis 14400 constellation: 128 unique mappings and nearest decisions pass");return 0;}
