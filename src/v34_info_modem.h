#ifndef SOFTMODEM_V34_INFO_MODEM_H
#define SOFTMODEM_V34_INFO_MODEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V34_INFO_MODEM_MAX_BITS 109u

typedef enum {
    V34_INFO_CALL_MODEM = 0,
    V34_INFO_ANSWER_MODEM = 1
} v34_info_modem_role;

typedef struct {
    v34_info_modem_role role;
    uint8_t frame[(V34_INFO_MODEM_MAX_BITS + 7u) / 8u];
    size_t bit_count;
    size_t bit_at;
    unsigned bit_clock;
    double carrier_phase;
    double guard_phase;
    int polarity;
    bool active;
} v34_info_modem_tx;

typedef struct {
    v34_info_modem_role remote_role;
    uint8_t frame[(V34_INFO_MODEM_MAX_BITS + 7u) / 8u];
    size_t expected_bits;
    size_t bit_count;
    unsigned bit_clock;
    double carrier_phase;
    double i_acc;
    double q_acc;
    double previous_i;
    double previous_q;
    bool carrier_seen;
    bool have_reference;
    bool ready;
} v34_info_modem_rx;

void v34_info_modem_tx_init(v34_info_modem_tx *tx,
                            v34_info_modem_role role);
bool v34_info_modem_tx_start(v34_info_modem_tx *tx, const uint8_t *frame,
                             size_t bit_count);
void v34_info_modem_tx_generate(v34_info_modem_tx *tx, uint8_t *pcma,
                                size_t sample_count);
bool v34_info_modem_tx_done(const v34_info_modem_tx *tx);

void v34_info_modem_rx_init(v34_info_modem_rx *rx,
                            v34_info_modem_role remote_role,
                            size_t expected_bits);
void v34_info_modem_rx_process(v34_info_modem_rx *rx, const uint8_t *pcma,
                               size_t sample_count);
bool v34_info_modem_rx_ready(const v34_info_modem_rx *rx);
bool v34_info_modem_rx_read(v34_info_modem_rx *rx, uint8_t *frame,
                            size_t capacity, size_t *bit_count);

#endif
