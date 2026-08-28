#include "ansam.h"
#include <float.h>
#include <math.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void ansam_generator_init(struct ansam_generator*g){memset(g,0,sizeof *g);}
void ansam_generate(struct ansam_generator*g,int16_t*out,size_t n){for(size_t k=0;k<n;k++){double t=(double)g->samples/8000.0,env=1.0+0.2*sin(2*M_PI*15.0*t);int sign=((g->samples/3600)&1)?-1:1;g->phase+=2*M_PI*2100.0/8000.0;if(g->phase>=2*M_PI)g->phase-=2*M_PI;out[k]=(int16_t)(sign*env*sin(g->phase)*9000.0);g->samples++;}}
void ansam_detector_init(struct ansam_detector*d){memset(d,0,sizeof *d);d->min_amp=DBL_MAX;}
static void finish(struct ansam_detector*d){double amp=hypot(d->i,d->q),phase=atan2(d->q,d->i);if(amp<d->min_amp)d->min_amp=amp;if(amp>d->max_amp)d->max_amp=amp;if(d->have_phase){double change=phase-d->previous_phase;while(change>M_PI)change-=2*M_PI;while(change<-M_PI)change+=2*M_PI;if(fabs(change)>2.2)d->reversals++;}d->previous_phase=phase;d->have_phase=1;d->blocks++;if(d->blocks>=100&&d->reversals>=1&&d->min_amp>0&&d->max_amp/d->min_amp>1.30)d->present=1;d->i=d->q=d->total=0;d->block=0;}
void ansam_detect(struct ansam_detector*d,const int16_t*in,size_t n){for(size_t k=0;k<n;k++){double p=2*M_PI*2100.0*(double)(d->blocks*80+d->block)/8000.0,x=in[k];d->i+=x*cos(p);d->q-=x*sin(p);d->total+=x*x;if(++d->block==80)finish(d);}}
int ansam_present(const struct ansam_detector*d){return d->present;}
