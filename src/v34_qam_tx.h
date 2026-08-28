#ifndef SOFTMODEM_V34_QAM_TX_H
#define SOFTMODEM_V34_QAM_TX_H

#include "v34_data_mapper.h"
#include "v34_caps.h"

typedef struct {
    double carrier_phase;
    double carrier_step;
    double coordinate_scale;
    v34_point point;
} v34_qam_tx;

bool v34_qam_tx_init(v34_qam_tx *tx, v34_symbol_rate rate,
                     bool high_carrier, unsigned sample_rate,
                     double coordinate_scale);
void v34_qam_tx_set_point(v34_qam_tx *tx, v34_point point);
int16_t v34_qam_tx_sample(v34_qam_tx *tx);
uint8_t v34_qam_tx_pcma(v34_qam_tx *tx);

#endif
