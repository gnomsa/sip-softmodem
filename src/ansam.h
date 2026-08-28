#ifndef SIP_SOFTMODEM_ANSAM_H
#define SIP_SOFTMODEM_ANSAM_H
#include <stddef.h>
#include <stdint.h>
struct ansam_generator {uint64_t samples;double phase;};
struct ansam_detector {double i,q,total;size_t block;double previous_phase,min_amp,max_amp;unsigned blocks,reversals;int have_phase,present;};
void ansam_generator_init(struct ansam_generator*g);
void ansam_generate(struct ansam_generator*g,int16_t*out,size_t count);
void ansam_detector_init(struct ansam_detector*d);
void ansam_detect(struct ansam_detector*d,const int16_t*in,size_t count);
int ansam_present(const struct ansam_detector*d);
#endif
