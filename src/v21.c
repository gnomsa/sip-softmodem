#include "v21.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define QMASK 4095u

void v21_init(struct v21 *v) { memset(v,0,sizeof *v); }

size_t v21_write(struct v21 *v,const uint8_t *d,size_t n) {
    size_t done=0;
    while(done<n && ((v->tx_tail+1)&QMASK)!=v->tx_head) {
        v->tx_queue[v->tx_tail]=d[done++]; v->tx_tail=(v->tx_tail+1)&QMASK;
    }
    return done;
}

size_t v21_read(struct v21 *v,uint8_t *d,size_t n) {
    size_t done=0;
    while(done<n && v->rx_head!=v->rx_tail) {
        d[done++]=v->rx_queue[v->rx_head]; v->rx_head=(v->rx_head+1)&QMASK;
    }
    return done;
}

static int tx_bit(struct v21 *v) {
    if (!v->tx_bits) {
        if (v->tx_head==v->tx_tail) return 1; /* continuous mark carrier */
        unsigned byte=v->tx_queue[v->tx_head]; v->tx_head=(v->tx_head+1)&QMASK;
        v->tx_frame=(1u<<9)|(byte<<1); /* start=0, 8 data LSB first, stop=1 */
        v->tx_bits=10;
    }
    return (v->tx_frame>>(10-v->tx_bits--))&1u;
}

void v21_generate(struct v21 *v,int16_t *out,size_t n) {
    for(size_t i=0;i<n;i++) {
        if (v->tx_clock<=0.0) { v->tx_bit=tx_bit(v); v->tx_clock += (double)V21_RATE/V21_BAUD; }
        /* Answer-channel: mark 1650 Hz, space 1850 Hz. */
        double freq=v->tx_bit?1650.0:1850.0;
        v->tx_phase += 2.0*M_PI*freq/V21_RATE;
        if(v->tx_phase>=2.0*M_PI)v->tx_phase-=2.0*M_PI;
        out[i]=(int16_t)(sin(v->tx_phase)*11000.0); v->tx_clock-=1.0;
    }
}

static void receive_bit(struct v21 *v,int bit) {
    if (!v->rx_receiving) { if (!bit) { v->rx_receiving=1; v->rx_bits=0; v->rx_frame=0; } return; }
    if (v->rx_bits<8) v->rx_frame |= (unsigned)bit<<v->rx_bits++;
    else { /* stop bit */
        if (bit) { size_t next=(v->rx_tail+1)&QMASK; if(next!=v->rx_head){v->rx_queue[v->rx_tail]=(uint8_t)v->rx_frame;v->rx_tail=next;} }
        v->rx_receiving=0;
    }
}

void v21_receive(struct v21 *v,const int16_t *in,size_t n) {
    for(size_t k=0;k<n;k++) {
        double x=in[k]/32768.0;
        /* The calling/originate modem transmits channel 1: mark 980, space 1180. */
        double pm=v->rx_phase*980.0, ps=v->rx_phase*1180.0;
        v->rx_mark_i+=x*cos(pm); v->rx_mark_q+=x*sin(pm);
        v->rx_space_i+=x*cos(ps); v->rx_space_q+=x*sin(ps);
        v->rx_phase += 2.0*M_PI/V21_RATE; v->rx_clock+=1.0;
        if(v->rx_clock >= (double)V21_RATE/V21_BAUD) {
            double mark=v->rx_mark_i*v->rx_mark_i+v->rx_mark_q*v->rx_mark_q;
            double space=v->rx_space_i*v->rx_space_i+v->rx_space_q*v->rx_space_q;
            receive_bit(v,mark>space);
            v->rx_mark_i=v->rx_mark_q=v->rx_space_i=v->rx_space_q=0.0;
            v->rx_clock-=(double)V21_RATE/V21_BAUD;
        }
    }
}
