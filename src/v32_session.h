#ifndef SIP_SOFTMODEM_V32_SESSION_H
#define SIP_SOFTMODEM_V32_SESSION_H

#include "v32_data.h"
#include "v32_e.h"
#include "v32_line.h"
#include "v32_qam.h"
#include "v32_rate.h"
#include "v32_retrain.h"
#include "v32_startup.h"
#include "v32_training.h"
#include "v32bis_data.h"
#include "v32bis_qam.h"
#include <stddef.h>
#include <stdint.h>

/* A rectangular 1800-Hz/2400-symbol/s acquisition repeats only after a
 * relatively long joint carrier/symbol phase cycle.  Cover enough sample
 * offsets to include every word alignment seen with 20-ms RTP starts. */
#define V32_STARTUP_SCANNERS 96
#define V32BIS_RX_TIMING_PHASES 10
#define V32BIS_RX_DIFFERENTIAL_STATES 4
#define V32BIS_RX_ALIGNMENT_RADIUS 8
#define V32BIS_RX_ALIGNMENTS (2*V32BIS_RX_ALIGNMENT_RADIUS+1)
#define V32BIS_RX_CANDIDATES \
    (V32BIS_RX_TIMING_PHASES*V32BIS_RX_DIFFERENTIAL_STATES* \
     V32BIS_RX_ALIGNMENTS)
#define V32BIS_RX_MAX_B1_EVM 0.60
#define V32BIS_RX_RETRAIN_WAIT_SYMBOLS 384

struct v32_startup_scanner {
    struct v32_line line;
    struct v32_rate_rx rate;
    enum v32_carrier_state last;
    unsigned skip, symbols;
    int rate_ready;
};

struct v32bis_rx_equalizer {
    struct v32bis_sample history[9],weights[9];
    unsigned history_count,history_at;
    int ready;
    double error,power,h_re,h_im;
    double carrier_cross_i,carrier_cross_q;
    double carrier_correlation_i,carrier_correlation_q;
    unsigned phase_count;
    double carrier_phase,carrier_step,carrier_confidence;
    int carrier_enabled;
};

/* Composite V.32 media session.  V.8 has already selected the V.32 family
 * before this object starts. */
struct v32_session {
    enum v32_std_role role;
    struct v32_line line, retrain_monitor;
    struct v32_qam qam;
    struct v32_training training;
    struct v32_rate_tx rate_tx;
    struct v32_rate_rx rate_rx;
    struct v32_rate_tx e_tx;
    struct v32_e_rx e_rx;
    struct v32_startup startup;
    struct v32_retrain retrain;
    struct v32_data data;
    struct v32bis_data bis_data;
    struct v32bis_data bis_rx_reference;
    struct v32bis_qam bis_qam;
    enum v32_carrier_state last_tx, last_rx;
    uint64_t tx_samples, rx_samples;
    unsigned tx_symbols, rx_symbols, tx_marking, rx_marking;
    uint8_t pending[8192];
    size_t pending_head, pending_tail;
    int rate_tx_ready, rate_rx_ready, e_tx_ready, e_rx_ready;
    int remote_r3, remote_e, data_ready, rx_data_ready, standard_startup;
    int local_e_complete, bis_qam_ready;
    unsigned local_e_symbols, bis_rx_known, bis_rx_skipped;
    struct v32bis_sample bis_rx_expected[128];
    struct v32bis_sample
        bis_rx_candidate_expected[V32BIS_RX_DIFFERENTIAL_STATES][128];
    struct v32bis_rx_equalizer bis_rx_eq;
    struct v32bis_qam bis_rx_candidate_qam[V32BIS_RX_TIMING_PHASES];
    struct v32bis_rx_equalizer bis_rx_candidate_eq[V32BIS_RX_CANDIDATES];
    unsigned bis_rx_candidate_seen[V32BIS_RX_TIMING_PHASES];
    unsigned bis_rx_candidate_target;
    int bis_rx_candidate_active,bis_rx_selected_phase;
    int bis_rx_selected_alignment;
    int bis_rx_acquisition_complete,bis_rx_acquisition_ok;
    unsigned bis_rx_retrain_ab_at_failure;
    unsigned bis_rx_reject_wait_symbols;
    unsigned bis_rx_selected_previous;
    unsigned startup_transition_symbols, startup_timer_symbols;
    unsigned startup_reversals, startup_tone_blocks, startup_tone_misses;
    unsigned startup_echo_symbols, startup_training_symbols;
    unsigned startup_rate_symbols;
    double startup_tone_i, startup_tone_q;
    int startup_tone_valid, startup_tone_bin, startup_scanner_selected;
    struct v32_startup_scanner startup_scanner[V32_STARTUP_SCANNERS];
};

void v32_session_init(struct v32_session *s, enum v32_std_role role,
                      int allow_4800, int allow_9600);
void v32bis_session_init(struct v32_session*s,enum v32_std_role role,int max_rate);
/* Enter the V.32 GSTN start-up sequence after V.8/V.25 has selected V.32.
 * The default initializer retains the deterministic training shortcut used by
 * the local DSP regression tests. */
void v32_session_start_standard(struct v32_session *s);
void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count);
void v32_session_receive(struct v32_session *s, const int16_t *pcm, size_t count);
void v32_session_media_gap(struct v32_session *s);
size_t v32_session_write(struct v32_session *s, const uint8_t *bytes, size_t count);
size_t v32_session_read(struct v32_session *s, uint8_t *bytes, size_t capacity);
int v32_session_connected(const struct v32_session *s);
int v32_session_rate(const struct v32_session *s);
size_t v32_session_pending(const struct v32_session *s);

#endif
