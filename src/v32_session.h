#ifndef SIP_SOFTMODEM_V32_SESSION_H
#define SIP_SOFTMODEM_V32_SESSION_H

#include "v32_data.h"
#include "v32_line.h"
#include "v32_qam.h"
#include "v32_rate.h"
#include "v32_startup.h"
#include "v32_training.h"
#include <stddef.h>
#include <stdint.h>

/* Composite V.32 media session.  V.8 has already selected the V.32 family
 * before this object starts. */
struct v32_session {
    enum v32_std_role role;
    struct v32_line line;
    struct v32_qam qam;
    struct v32_training training;
    struct v32_rate_tx rate_tx;
    struct v32_rate_rx rate_rx;
    struct v32_startup startup;
    struct v32_data data;
    enum v32_carrier_state last_tx, last_rx;
    uint64_t tx_samples, rx_samples;
    unsigned tx_symbols, rx_symbols;
    int rate_tx_ready, rate_rx_ready, data_ready;
};

void v32_session_init(struct v32_session *s, enum v32_std_role role,
                      int allow_4800, int allow_9600);
void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count);
void v32_session_receive(struct v32_session *s, const int16_t *pcm, size_t count);
size_t v32_session_write(struct v32_session *s, const uint8_t *bytes, size_t count);
size_t v32_session_read(struct v32_session *s, uint8_t *bytes, size_t capacity);
int v32_session_connected(const struct v32_session *s);
int v32_session_rate(const struct v32_session *s);

#endif
