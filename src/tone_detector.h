#ifndef SIP_SOFTMODEM_TONE_DETECTOR_H
#define SIP_SOFTMODEM_TONE_DETECTOR_H
#include <stddef.h>
#include <stdint.h>

struct tone_detector {
    double frequency;
    double s1, s2, total;
    size_t samples, block_samples;
    unsigned present_blocks, absent_blocks;
    int present;
};

void tone_detector_init(struct tone_detector *d, double frequency, size_t block_samples);
void tone_detector_process(struct tone_detector *d, const int16_t *samples, size_t count);
int tone_detector_present(const struct tone_detector *d);
#endif
