#include "v32_session.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static enum v32_std_role other(enum v32_std_role r)
{
    return r == V32_STD_CALL ? V32_STD_ANSWER : V32_STD_CALL;
}

static void begin_data(struct v32_session *s);

static void startup_scanners_init(struct v32_session *s)
{
    for(unsigned i=0;i<V32_STARTUP_SCANNERS;i++){
        struct v32_startup_scanner *scanner=&s->startup_scanner[i];
        v32_line_init(&scanner->line);scanner->last=V32_STATE_A;
        scanner->skip=i;scanner->symbols=0;scanner->rate_ready=0;
    }
}

static void media_init(struct v32_session *s,enum v32_std_role role,
                       unsigned rates)
{
    s->role=role;s->last_tx=s->last_rx=V32_STATE_A;
    s->tx_samples=s->rx_samples=0;s->tx_symbols=s->rx_symbols=0;
    s->tx_marking=s->rx_marking=0;
    s->rate_tx_ready=s->rate_rx_ready=s->e_tx_ready=s->e_rx_ready=0;
    s->remote_r3=s->remote_e=s->data_ready=0;
    v32_line_init(&s->line);v32_line_init(&s->retrain_monitor);
    v32_qam_init(&s->qam);v32_training_init(&s->training,role);
    if(rates&(V32_RATE_7200|V32_RATE_12000|V32_RATE_14400))v32bis_startup_init(&s->startup,role,rates);
    else v32_startup_init(&s->startup,role,!!(rates&V32_RATE_4800),!!(rates&V32_RATE_9600));
    v32_retrain_init(&s->retrain);s->startup.phase=V32_START_RATE_1;
}

void v32_session_init(struct v32_session *s, enum v32_std_role role,
                      int allow_4800, int allow_9600)
{
    *s = (struct v32_session){0};
    media_init(s,role,(allow_4800?V32_RATE_4800:0)|(allow_9600?V32_RATE_9600:0));
}

void v32bis_session_init(struct v32_session*s,enum v32_std_role role,int max_rate)
{
    unsigned rates=V32_RATE_4800;if(max_rate>=7200)rates|=V32_RATE_7200;if(max_rate>=9600)rates|=V32_RATE_9600;if(max_rate>=12000)rates|=V32_RATE_12000;if(max_rate>=14400)rates|=V32_RATE_14400;
    *s=(struct v32_session){0};media_init(s,role,rates);
}

void v32_session_start_standard(struct v32_session *s)
{
    s->standard_startup = 1;
    s->tx_symbols = s->rx_symbols = 0;
    s->startup_transition_symbols = s->startup_timer_symbols = 0;
    s->startup_echo_symbols=s->startup_training_symbols=0;
    s->startup_rate_symbols=0;
    s->startup_reversals = s->startup_tone_blocks = 0;
    s->startup_tone_misses=0;
    s->startup_tone_i = s->startup_tone_q = 0.0;
    s->startup_tone_valid = 0;s->startup_tone_bin=-1;
    s->startup_scanner_selected=-1;
    startup_scanners_init(s);
    v32_line_init(&s->line);
    v32_line_set_pulse_shaped(&s->line,1);
    if (s->role == V32_STD_CALL) {
        s->startup.phase = V32_START_WAIT_ANSWER_TONE;
        v32_startup_answer_tone(&s->startup);
    } else {
        s->startup.phase = V32_START_AC;
        s->startup.symbols = 128;
    }
}

static void startup_tone_receive(struct v32_session *s,
                                 const int16_t *pcm,size_t count)
{
    double energy=0.0,tone_i[2]={0.0,0.0},tone_q[2]={0.0,0.0};
    double tone_power[2]={0.0,0.0};
    const double frequencies[2]={600.0,3000.0};
    for(size_t k=0;k<count;k++){double x=pcm[k];energy+=x*x;}
    for(size_t f=0;f<2;f++){
        for(size_t k=0;k<count;k++){
            double p=2.0*M_PI*frequencies[f]*(double)k/8000.0;
            tone_i[f]+=(double)pcm[k]*cos(p);tone_q[f]-=(double)pcm[k]*sin(p);
        }
        tone_power[f]=tone_i[f]*tone_i[f]+tone_q[f]*tone_q[f];
    }
    int bin=s->startup_tone_bin;
    if(bin<0)bin=tone_power[1]>tone_power[0]?1:0;
    double best_i=tone_i[bin],best_q=tone_q[bin],best_power=tone_power[bin];
    if(energy<1.0||best_power<0.10*energy*(double)count){
        if(++s->startup_tone_misses>5){s->startup_tone_valid=0;
            s->startup_tone_blocks=0;s->startup_tone_bin=-1;}
        return;
    }
    s->startup_tone_misses=0;
    if(!s->startup_tone_valid){
        s->startup_tone_i=best_i;s->startup_tone_q=best_q;
        s->startup_tone_valid=1;s->startup_tone_blocks=1;
        s->startup_tone_bin=bin;return;
    }
    {
        double old_power=s->startup_tone_i*s->startup_tone_i+s->startup_tone_q*s->startup_tone_q;
        double similarity=(best_i*s->startup_tone_i+best_q*s->startup_tone_q)/sqrt(best_power*old_power);
        if(s->startup_tone_blocks>=2&&similarity<-0.70){
            if(s->startup.phase==V32_START_AA&&s->startup_reversals==0){
                s->startup_reversals=1;s->startup_transition_symbols=64;
                s->startup_timer_symbols=0;
            }else if(s->startup.phase==V32_START_CC&&s->startup_reversals==1){
                s->startup_reversals=2;
                v32_startup_phase_reversal(&s->startup);
                v32_line_init(&s->line);
                v32_line_set_pulse_shaped(&s->line,1);
                s->rx_symbols=0;
                startup_scanners_init(s);
                v32_startup_remote_s(&s->startup);
            }
            s->startup_tone_i=best_i;s->startup_tone_q=best_q;
            s->startup_tone_blocks=1;return;
        }
        if(similarity>0.70&&s->startup_tone_blocks<3)s->startup_tone_blocks++;
        else if(similarity<=0.70&&similarity>=-0.70)return;
        }
}

static int startup_accept_r1(struct v32_session *s,uint16_t word)
{
    unsigned remote_rates=0;int trellis=0,bis=0;
    if(v32bis_rate_decode(word,&remote_rates,&trellis,&bis)<0)return 0;
    unsigned common=remote_rates&s->startup.allowed_rates;
    int selected=v32_highest_rate(common);if(!selected)return 0;
    s->startup.remote_rate_word=word;s->startup.selected_rate=selected;
    s->startup.bis_selected=0;
    s->startup.local_rate_word=v32_std_rate_word(
        !!(common&V32_RATE_4800),!!(common&V32_RATE_9600),0);
    s->startup.phase=V32_START_TRAIN_2;
    s->startup_echo_symbols=s->startup_timer_symbols;
    s->startup_training_symbols=0;s->tx_symbols=0;
    v32_training_init(&s->training,s->role);v32_line_init(&s->line);
    v32_line_set_pulse_shaped(&s->line,1);
    startup_scanners_init(s);
    s->startup_scanner_selected=-1;
    return 1;
}

static int startup_accept_r3(struct v32_session *s,unsigned scanner_index,
                             uint16_t word)
{
    unsigned remote_rates=0;int trellis=0,bis=0;
    if(v32bis_rate_decode(word,&remote_rates,&trellis,&bis)<0)return 0;
    unsigned selected=s->startup.selected_rate==9600?V32_RATE_9600:
                      s->startup.selected_rate==4800?V32_RATE_4800:0;
    if(remote_rates!=selected||trellis||bis)return 0;
    s->startup.remote_rate_word=word;
    s->remote_r3=1;
    s->startup_scanner_selected=(int)scanner_index;
    v32_e_rx_continue(&s->e_rx,&s->startup_scanner[scanner_index].rate);
    s->e_rx_ready=1;
    return 1;
}

static void startup_r1_receive(struct v32_session *s,const int16_t *pcm,size_t count)
{
    enum v32_carrier_state states[128];
    for(unsigned c=0;c<V32_STARTUP_SCANNERS;c++){
        struct v32_startup_scanner *scanner=&s->startup_scanner[c];
        size_t offset=scanner->skip<count?scanner->skip:count;
        scanner->skip-=(unsigned)offset;
        if(offset<count)v32_line_receive(&scanner->line,pcm+offset,count-offset);
        size_t n;
        while((n=v32_line_read(&scanner->line,states,128))!=0){
            for(size_t i=0;i<n;i++){
                scanner->last=states[i];scanner->symbols++;
                if(scanner->symbols<1552)continue;
                if(!scanner->rate_ready){
                    v32_rate_rx_init(&scanner->rate,other(s->role),scanner->last);
                    scanner->rate_ready=1;
                    if(s->startup.phase==V32_START_TRAIN_1)
                        s->startup.phase=V32_START_RATE_1;
                    continue;
                }
                uint16_t word;
                if(v32_rate_rx_put(&scanner->rate,states[i],&word)&&
                   startup_accept_r1(s,word))return;
            }
        }
    }
}

static void startup_r3_receive(struct v32_session *s,const int16_t *pcm,
                               size_t count)
{
    enum v32_carrier_state states[128];
    if(s->startup_scanner_selected>=0){
        struct v32_startup_scanner *scanner=
            &s->startup_scanner[s->startup_scanner_selected];
        v32_line_receive(&scanner->line,pcm,count);size_t n;
        while((n=v32_line_read(&scanner->line,states,128))!=0){
            for(size_t i=0;i<n;i++){
                scanner->last=states[i];
                int rate,trellis;
                if(s->e_rx_ready&&
                   v32_e_rx_put(&s->e_rx,states[i],&rate,&trellis)&&
                   rate==s->startup.selected_rate&&!trellis){
                    s->remote_e=1;
                    v32_startup_remote_e(&s->startup);
                    begin_data(s);
                    return;
                }
            }
        }
        return;
    }
    for(unsigned c=0;c<V32_STARTUP_SCANNERS;c++){
        struct v32_startup_scanner *scanner=&s->startup_scanner[c];
        size_t offset=scanner->skip<count?scanner->skip:count;
        scanner->skip-=(unsigned)offset;
        if(offset<count)v32_line_receive(&scanner->line,pcm+offset,count-offset);
        size_t n;
        while((n=v32_line_read(&scanner->line,states,128))!=0){
            for(size_t i=0;i<n;i++){
                scanner->last=states[i];scanner->symbols++;
                if(scanner->symbols<1552)continue;
                if(!scanner->rate_ready){
                    v32_rate_rx_init(&scanner->rate,other(s->role),scanner->last);
                    scanner->rate_ready=1;continue;
                }
                uint16_t word;
                if(v32_rate_rx_put(&scanner->rate,states[i],&word)&&
                   startup_accept_r3(s,c,word)){
                    for(size_t j=i+1;j<n;j++){
                        scanner->last=states[j];
                        int rate,trellis;
                        if(v32_e_rx_put(&s->e_rx,states[j],&rate,&trellis)&&
                           rate==s->startup.selected_rate&&!trellis){
                            s->remote_e=1;
                            v32_startup_remote_e(&s->startup);
                            begin_data(s);
                            return;
                        }
                    }
                    return;
                }
            }
        }
    }
}

static void pump_pending(struct v32_session*s)
{
    while(v32_session_connected(s)&&s->pending_head!=s->pending_tail){
        size_t end=s->pending_tail>s->pending_head?s->pending_tail:sizeof s->pending;
        size_t n=s->startup.bis_selected?v32bis_data_write(&s->bis_data,s->pending+s->pending_head,end-s->pending_head):v32_data_write(&s->data,s->pending+s->pending_head,end-s->pending_head);
        s->pending_head=(s->pending_head+n)%sizeof s->pending;if(!n)break;
    }
}

static void queue_line_symbols(struct v32_session *s, size_t count)
{
    enum v32_carrier_state states[64];
    while (count) {
        size_t n = count > 64 ? 64 : count;
        for (size_t i = 0; i < n; i++) {
            if (s->tx_symbols < 1552) {
                states[i] = v32_training_next(&s->training);
            } else if (!s->startup.selected_rate) {
                if (!s->rate_tx_ready) {
                    v32_rate_tx_init(&s->rate_tx, s->role,
                                     s->startup.local_rate_word, s->last_tx);
                    s->rate_tx_ready = 1;
                }
                states[i] = v32_rate_tx_next(&s->rate_tx);
            } else if (!s->remote_e) {
                if (!s->e_tx_ready) {
                    v32_rate_tx_init(&s->e_tx, s->role,
                                     s->startup.bis_selected?v32bis_e_word(s->startup.selected_rate,1):v32_std_e_word(s->startup.selected_rate, 0),
                                     s->last_tx);
                    s->e_tx_ready = 1;
                }
                states[i] = v32_rate_tx_next(&s->e_tx);
            } else {
                states[i] = v32_data_next_4800(&s->data);
                if (s->tx_marking < 128) s->tx_marking++;
            }
            s->last_tx = states[i];
            s->tx_symbols++;
        }
        (void)v32_line_write(&s->line, states, n);
        count -= n;
    }
}

static void queue_standard_training(struct v32_session *s,size_t count)
{
    enum v32_carrier_state states[64];
    while(count){
        size_t n=count>64?64:count;
        for(size_t i=0;i<n;i++){
            if(s->startup_echo_symbols){
                states[i]=(s->tx_symbols&1u)?V32_STATE_B:V32_STATE_A;
                s->startup_echo_symbols--;
            }else if(s->startup_training_symbols<1552){
                states[i]=v32_training_next(&s->training);
                s->startup_training_symbols++;
            }else{
                if(s->startup.phase==V32_START_TRAIN_2){
                    s->startup.phase=V32_START_RATE_2;
                    s->startup_rate_symbols=0;
                    v32_rate_tx_init(&s->rate_tx,s->role,
                                     s->startup.local_rate_word,s->last_tx);
                    s->rate_tx_ready=1;
                }
                if(s->remote_r3&&s->startup_rate_symbols>=32){
                    s->startup.phase=V32_START_E;
                    if(!s->e_tx_ready){
                        v32_rate_tx_init(&s->e_tx,s->role,
                            v32_std_e_word(s->startup.selected_rate,0),
                            s->last_tx);
                        s->e_tx_ready=1;
                    }
                    states[i]=v32_rate_tx_next(&s->e_tx);
                }else{
                    states[i]=v32_rate_tx_next(&s->rate_tx);
                    s->startup_rate_symbols++;
                }
            }
            s->last_tx=states[i];s->tx_symbols++;
        }
        (void)v32_line_write(&s->line,states,n);count-=n;
    }
}

static void begin_data(struct v32_session *s)
{
    if (s->data_ready || !s->startup.selected_rate || !s->remote_e) return;
    if(s->startup.bis_selected){(void)v32bis_data_init(&s->bis_data,s->role,s->startup.selected_rate);(void)v32bis_qam_init(&s->bis_qam,s->startup.selected_rate);}
    else{
        enum v32_carrier_state previous_rx=V32_STATE_A;
        if(s->standard_startup&&s->startup_scanner_selected>=0)
            previous_rx=s->startup_scanner[s->startup_scanner_selected].last;
        v32_data_init(&s->data,s->role,
                      s->standard_startup?s->last_tx:V32_STATE_A,
                      previous_rx);
        v32_qam_init(&s->qam);
        if(s->standard_startup){
            /* The scrambler, differential encoder, carrier phase and symbol
             * clock are continuous across the single E word into B1. */
            s->data.tx_scr=s->e_tx.scr;
            s->data.rx_descr=s->e_rx.descr;
            s->qam.tx_samples=s->line.tx_samples;
            if(s->startup_scanner_selected>=0){
                const struct v32_line *rx=
                    &s->startup_scanner[s->startup_scanner_selected].line;
                s->qam.rx_samples=rx->rx_samples;
                s->qam.rx_clock=rx->rx_clock;
                s->qam.rx_i=rx->rx_i;s->qam.rx_q=rx->rx_q;
                s->qam.rx_cc=rx->rx_cc;s->qam.rx_ss=rx->rx_ss;
                s->qam.rx_cs=rx->rx_cs;
            }
        }
    }
    s->data_ready = 1;
}

void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count)
{
    if (s->standard_startup &&
        (s->startup.phase == V32_START_AA ||
         s->startup.phase == V32_START_AC ||
         s->startup.phase == V32_START_CA ||
         s->startup.phase == V32_START_CC)) {
        enum v32_carrier_state states[64];
        size_t remaining = count * 2400 / 8000;
        while (remaining) {
            size_t n = remaining > 64 ? 64 : remaining;
            for (size_t i = 0; i < n; i++) {
                if(s->startup_reversals==1)s->startup_timer_symbols++;
                if(s->startup.phase==V32_START_AA&&s->startup_transition_symbols){
                    s->startup_transition_symbols--;
                    if(!s->startup_transition_symbols)
                        v32_startup_phase_reversal(&s->startup);
                }
                if (s->startup.phase == V32_START_AA)
                    states[i] = V32_STATE_A;
                else if (s->startup.phase == V32_START_CC)
                    states[i] = V32_STATE_C;
                else {
                    int reverse = s->startup.phase == V32_START_CA;
                    states[i] = ((s->tx_symbols + (unsigned)reverse) & 1u) ?
                                V32_STATE_C : V32_STATE_A;
                }
                s->last_tx = states[i];
                s->tx_symbols++;
            }
            (void)v32_line_write(&s->line, states, n);
            remaining -= n;
        }
        v32_line_generate(&s->line, pcm, count);
        s->tx_samples += count;
        return;
    }
    if(s->standard_startup&&s->startup.phase==V32_START_WAIT_REMOTE_S){
        memset(pcm,0,count*sizeof *pcm);s->tx_samples+=count;return;
    }
    if(s->standard_startup&&(s->startup.phase==V32_START_TRAIN_1||
                            s->startup.phase==V32_START_RATE_1)){
        memset(pcm,0,count*sizeof *pcm);s->tx_samples+=count;return;
    }
    if(s->standard_startup&&(s->startup.phase==V32_START_TRAIN_2||
                            s->startup.phase==V32_START_RATE_2)){
        queue_standard_training(s,count*2400/8000);
        v32_line_generate(&s->line,pcm,count);s->tx_samples+=count;return;
    }
    if (s->retrain.state == V32_RETRAIN_RESTART) {
        unsigned rates=s->startup.allowed_rates;media_init(s,s->role,rates);
    }
    if (s->retrain.state == V32_RETRAIN_REQUEST || s->retrain.state == V32_RETRAIN_ACK) {
        enum v32_carrier_state states[64]; size_t n=count*2400/8000;
        for(size_t i=0;i<n;i++)states[i]=v32_retrain_next(&s->retrain);
        (void)v32_line_write(&s->line,states,n);v32_line_generate(&s->line,pcm,count);
        s->tx_samples+=count;return;
    }
    begin_data(s);
    pump_pending(s);
    if(s->data_ready&&s->startup.bis_selected){uint8_t symbols[64];size_t n=count*2400/8000;for(size_t i=0;i<n;i++){symbols[i]=(uint8_t)v32bis_data_next(&s->bis_data);if(s->tx_marking<128)s->tx_marking++;}(void)v32bis_qam_write(&s->bis_qam,symbols,n);v32bis_qam_generate(&s->bis_qam,pcm,count);
    } else if (s->data_ready && s->startup.selected_rate == 9600) {
            uint8_t symbols[64]; size_t n = count * 2400 / 8000;
            for (size_t i = 0; i < n; i++) {
                symbols[i] = v32_data_next_9600(&s->data);
                if (s->tx_marking < 128) s->tx_marking++;
            }
            (void)v32_qam_write(&s->qam, symbols, n);
            v32_qam_generate(&s->qam, pcm, count);
    } else {
        queue_line_symbols(s, count * 2400 / 8000);
        v32_line_generate(&s->line, pcm, count);
    }
    s->tx_samples += count;
}

void v32_session_media_gap(struct v32_session *s)
{
    if(v32_session_connected(s))v32_retrain_request(&s->retrain);
}

static int monitor_retrain(struct v32_session *s,const int16_t *pcm,size_t count)
{
    if(!v32_session_connected(s) && s->retrain.state==V32_RETRAIN_IDLE)return 0;
    enum v32_carrier_state states[128];v32_line_receive(&s->retrain_monitor,pcm,count);
    size_t n;while((n=v32_line_read(&s->retrain_monitor,states,128))!=0)
        for(size_t i=0;i<n;i++)if(v32_retrain_put(&s->retrain,states[i])){
            if(s->retrain.state==V32_RETRAIN_RESTART){
                unsigned rates=s->startup.allowed_rates;media_init(s,s->role,rates);
            }
            return 1;
        }
    return s->retrain.state!=V32_RETRAIN_IDLE;
}

static void consume_line(struct v32_session *s)
{
    enum v32_carrier_state states[128]; size_t n;
    while ((n = v32_line_read(&s->line, states, 128)) != 0) {
        for (size_t i = 0; i < n; i++) {
            if (s->rx_symbols < 1552) {
                s->last_rx = states[i];
                s->rx_symbols++;
                if(s->standard_startup&&s->rx_symbols==1552&&
                   s->startup.phase==V32_START_TRAIN_1)
                    s->startup.phase=V32_START_RATE_1;
                continue;
            }
            if (!s->rate_rx_ready) {
                v32_rate_rx_init(&s->rate_rx, other(s->role), s->last_rx);
                s->rate_rx_ready = 1;
            }
            s->last_rx = states[i];
            s->rx_symbols++;
            if (!s->startup.selected_rate) {
                uint16_t word;
                if (v32_rate_rx_put(&s->rate_rx, states[i], &word)) {
                    if(s->standard_startup&&s->startup.phase==V32_START_RATE_1){
                        unsigned remote_rates=0;int trellis=0,bis=0;
                        if(v32bis_rate_decode(word,&remote_rates,&trellis,&bis)==0){
                            unsigned common=remote_rates&s->startup.allowed_rates;
                            int selected=v32_highest_rate(common);
                            if(selected){
                                s->startup.remote_rate_word=word;
                                s->startup.selected_rate=selected;
                                s->startup.bis_selected=0;
                                s->startup.local_rate_word=v32_std_rate_word(
                                    !!(common&V32_RATE_4800),
                                    !!(common&V32_RATE_9600),0);
                                s->startup.phase=V32_START_TRAIN_2;
                                s->startup_echo_symbols=s->startup_timer_symbols;
                                s->startup_training_symbols=0;
                                s->tx_symbols=0;v32_training_init(&s->training,s->role);
                                v32_line_init(&s->line);
                                return;
                            }
                        }
                        continue;
                    }
                    int selected = v32_startup_rate_word(&s->startup, word);
                    if (selected > 0) {
                        v32_e_rx_init(&s->e_rx, other(s->role), states[i]);
                        s->e_rx_ready = 1;
                    }
                }
            } else if (!s->remote_e && s->e_rx_ready) {
                int rate, trellis;
                if (v32_e_rx_put(&s->e_rx, states[i], &rate, &trellis) &&
                    rate == s->startup.selected_rate && trellis==!!s->startup.bis_selected) {
                    s->remote_e = 1;
                    begin_data(s);
                    /* The peer changes from E to marking on its next media
                     * block.  Discard the tail of this already received E
                     * block so it cannot advance the data descrambler. */
                    return;
                }
            } else if (s->data_ready && s->startup.selected_rate == 4800) {
                v32_data_put_4800(&s->data, states[i]);
                if (s->rx_marking < 128 && ++s->rx_marking == 128)
                    s->data.rh = s->data.rt;
            }
        }
    }
}

void v32_session_receive(struct v32_session *s, const int16_t *pcm, size_t count)
{
    if (s->standard_startup &&
        (s->startup.phase == V32_START_AA ||
         s->startup.phase == V32_START_AC ||
         s->startup.phase == V32_START_CA ||
         s->startup.phase == V32_START_CC)) {
        startup_tone_receive(s,pcm,count);s->rx_samples += count;
        return;
    }
    if(s->standard_startup&&s->startup.phase==V32_START_WAIT_REMOTE_S){
        s->rx_samples+=count;return;
    }
    if(s->standard_startup&&(s->startup.phase==V32_START_TRAIN_1||
                            s->startup.phase==V32_START_RATE_1)){
        startup_r1_receive(s,pcm,count);s->rx_samples+=count;return;
    }
    if(s->standard_startup&&(s->startup.phase==V32_START_TRAIN_2||
                            s->startup.phase==V32_START_RATE_2)){
        startup_r3_receive(s,pcm,count);s->rx_samples+=count;return;
    }
    if(s->standard_startup&&s->startup.phase==V32_START_E&&
       !s->remote_e&&s->startup_scanner_selected>=0){
        startup_r3_receive(s,pcm,count);s->rx_samples+=count;return;
    }
    if(monitor_retrain(s,pcm,count)){s->rx_samples+=count;return;}
    begin_data(s);
    if(s->data_ready&&s->startup.bis_selected){struct v32bis_sample points[128];v32bis_qam_receive(&s->bis_qam,pcm,count);size_t n=v32bis_qam_read(&s->bis_qam,points,128);for(size_t i=0;i<n;i++){(void)v32bis_data_put(&s->bis_data,points[i].i,points[i].q);if(s->rx_marking<128&&++s->rx_marking==128)s->bis_data.rh=s->bis_data.rt;}
    } else if (s->data_ready && s->startup.selected_rate == 9600) {
        uint8_t symbols[128];
        v32_qam_receive(&s->qam, pcm, count);
        size_t n = v32_qam_read(&s->qam, symbols, 128);
        for (size_t i = 0; i < n; i++) {
            v32_data_put_9600(&s->data, symbols[i]);
            if (s->rx_marking < 128 && ++s->rx_marking == 128)
                s->data.rh = s->data.rt;
        }
    } else {
        v32_line_receive(&s->line, pcm, count);
        consume_line(s);
    }
    s->rx_samples += count;
}

size_t v32_session_write(struct v32_session *s, const uint8_t *b, size_t n)
{
    size_t z=0;
    while(z<n){
        size_t next=(s->pending_tail+1)%sizeof s->pending;
        if(next==s->pending_head)break;
        s->pending[s->pending_tail]=b[z++];s->pending_tail=next;
    }
    pump_pending(s);
    return z;
}
size_t v32_session_read(struct v32_session *s, uint8_t *b, size_t n)
{ return s->data_ready ? (s->startup.bis_selected?v32bis_data_read(&s->bis_data,b,n):v32_data_read(&s->data, b, n)) : 0; }
int v32_session_connected(const struct v32_session *s)
{ return s->retrain.state==V32_RETRAIN_IDLE && s->data_ready && s->tx_marking >= 128 && s->rx_marking >= 128; }
int v32_session_rate(const struct v32_session *s) { return s->startup.selected_rate; }
size_t v32_session_pending(const struct v32_session*s)
{return(s->pending_tail+sizeof s->pending-s->pending_head)%sizeof s->pending;}
