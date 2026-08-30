#include "v32_session.h"
#include "v32bis_map.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static enum v32_std_role other(enum v32_std_role r)
{
    return r == V32_STD_CALL ? V32_STD_ANSWER : V32_STD_CALL;
}

static unsigned rate_mask(int rate)
{
    return rate==4800?V32_RATE_4800:rate==7200?V32_RATE_7200:
           rate==9600?V32_RATE_9600:rate==12000?V32_RATE_12000:
           rate==14400?V32_RATE_14400:0;
}

static unsigned carrier_pair(enum v32_carrier_state state)
{
    static const unsigned pairs[4]={0,1,3,2};
    return pairs[state&3];
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
    s->remote_r3=s->remote_e=s->data_ready=s->rx_data_ready=0;
    s->local_e_complete=s->bis_qam_ready=0;
    s->local_e_symbols=s->bis_rx_known=s->bis_rx_skipped=0;
    memset(&s->bis_rx_eq,0,sizeof s->bis_rx_eq);
    memset(s->bis_rx_candidate_qam,0,sizeof s->bis_rx_candidate_qam);
    memset(s->bis_rx_candidate_eq,0,sizeof s->bis_rx_candidate_eq);
    memset(s->bis_rx_candidate_seen,0,sizeof s->bis_rx_candidate_seen);
    s->bis_rx_candidate_target=0;s->bis_rx_candidate_active=0;
    s->bis_rx_selected_phase=-1;s->bis_rx_selected_previous=0;
    s->bis_rx_selected_alignment=0;
    s->bis_rx_acquisition_complete=s->bis_rx_acquisition_ok=0;
    s->bis_rx_retrain_ab_at_failure=0;
    s->bis_rx_reject_wait_symbols=0;
    s->retrain_tone_blocks=0;
    v32_line_init(&s->line);v32_line_init(&s->retrain_monitor);
    v32_qam_init(&s->qam);v32_training_init(&s->training,role);
    if(rates&(V32_RATE_7200|V32_RATE_12000|V32_RATE_14400))v32bis_startup_init(&s->startup,role,rates);
    else v32_startup_init(&s->startup,role,!!(rates&V32_RATE_4800),!!(rates&V32_RATE_9600));
    v32_retrain_init(&s->retrain);s->startup.phase=V32_START_RATE_1;
}

static void standard_retrain_begin(struct v32_session *s,unsigned rates)
{
    enum v32_std_role role=s->role;
    media_init(s,role,rates);
    v32_session_start_standard(s);
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
    s->startup.bis_selected=bis&&trellis&&
        !!(common&(V32_RATE_7200|V32_RATE_12000|V32_RATE_14400));
    s->startup.local_rate_word=s->startup.bis_selected?
        v32bis_rate_word(common,1):
        v32_std_rate_word(!!(common&V32_RATE_4800),
                          !!(common&V32_RATE_9600),0);
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
    int selected=v32_highest_rate(remote_rates);
    if(!selected||remote_rates!=rate_mask(selected)||
       !(remote_rates&s->startup.allowed_rates))return 0;
    if(s->startup.bis_selected){if(!bis||!trellis||selected<7200)return 0;}
    else if(bis||trellis||selected>9600)return 0;
    s->startup.selected_rate=selected;
    s->startup.remote_rate_word=word;
    s->remote_r3=1;
    s->startup_scanner_selected=(int)scanner_index;
    v32_e_rx_continue(&s->e_rx,&s->startup_scanner[scanner_index].rate);
    v32_e_rx_expect(&s->e_rx,v32bis_e_word(selected,!!s->startup.bis_selected));
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
                   rate==s->startup.selected_rate&&
                   trellis==!!s->startup.bis_selected){
                    s->bis_rx_skipped=(unsigned)(n-i-1);
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
                           rate==s->startup.selected_rate&&
                           trellis==!!s->startup.bis_selected){
                            s->bis_rx_skipped=(unsigned)(n-j-1);
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
                if(s->startup.bis_selected&&!s->remote_r3&&
                   s->startup_rate_symbols>=7200&&
                   (s->startup_rate_symbols&7u)==0){
                    unsigned rates=0;int trellis=0,bis=0;
                    if(v32bis_rate_decode(s->startup.local_rate_word,&rates,
                                          &trellis,&bis)==0){
                        unsigned highest=rate_mask(v32_highest_rate(rates));
                        unsigned lower=rates&~highest;
                        if(lower){
                            s->startup.selected_rate=v32_highest_rate(lower);
                            s->startup.local_rate_word=
                                v32bis_rate_word(lower,1);
                            v32_rate_tx_init(&s->rate_tx,s->role,
                                s->startup.local_rate_word,s->last_tx);
                            s->startup_rate_symbols=0;
                        }
                    }
                }
                if(s->remote_r3&&s->startup_rate_symbols>=32&&
                   (s->startup_rate_symbols&7u)==0){
                    s->startup.phase=V32_START_E;
                    if(!s->e_tx_ready){
                        v32_rate_tx_continue(&s->e_tx,&s->rate_tx,
                            s->startup.bis_selected?
                            v32bis_e_word(s->startup.selected_rate,1):
                            v32_std_e_word(s->startup.selected_rate,0));
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
    enum{V32BIS_RX_HANDOFF_SYMBOLS=4};
    if (!s->startup.selected_rate || !s->remote_e) return;
    if(s->standard_startup&&s->startup.bis_selected){
        if(!s->data_ready||s->rx_data_ready)return;
        s->bis_data.rx_descr=s->e_rx.descr;
        s->bis_data.rx_previous=s->e_rx.previous;
        if(s->startup_scanner_selected>=0){
            const struct v32_startup_scanner *scanner=
                &s->startup_scanner[s->startup_scanner_selected];
            s->bis_qam.rx_samples=scanner->line.rx_samples;
        }
        s->bis_qam.rx_fir_at=s->bis_qam.rx_fir_count=0;
        s->bis_qam.rx_last_symbol=0;
        unsigned skipped=s->bis_rx_skipped+V32BIS_RX_HANDOFF_SYMBOLS;
        if(skipped>128)skipped=128;
        for(unsigned previous=0;
            previous<V32BIS_RX_DIFFERENTIAL_STATES;previous++){
            (void)v32bis_data_init(&s->bis_rx_reference,other(s->role),
                                   s->startup.selected_rate);
            s->bis_rx_reference.tx_scr=s->e_rx.descr;
            s->bis_rx_reference.tx_previous=previous;
            for(unsigned k=0;k<128;k++){
                unsigned label=v32bis_data_next(&s->bis_rx_reference);
                struct v32bis_point point;
                (void)v32bis_map_point(s->startup.selected_rate,label,&point);
                s->bis_rx_candidate_expected[previous][k]=
                    (struct v32bis_sample){point.i,point.q};
            }
        }
        memcpy(s->bis_rx_expected,
               s->bis_rx_candidate_expected[s->bis_data.rx_previous&3u],
               sizeof s->bis_rx_expected);
        s->bis_rx_known=128-skipped;s->rx_marking=skipped;
        memset(&s->bis_rx_eq,0,sizeof s->bis_rx_eq);
        s->bis_rx_candidate_target=skipped+64;
        if(s->bis_rx_candidate_target>120)s->bis_rx_candidate_target=120;
        for(unsigned phase=0;phase<V32BIS_RX_TIMING_PHASES;phase++){
            (void)v32bis_qam_init(&s->bis_rx_candidate_qam[phase],
                                  s->startup.selected_rate);
            v32bis_qam_set_pulse_shaped(&s->bis_rx_candidate_qam[phase],1);
            s->bis_rx_candidate_qam[phase].rx_clock=
                (double)phase/(double)V32BIS_RX_TIMING_PHASES;
            s->bis_rx_candidate_qam[phase].rx_samples=s->bis_qam.rx_samples;
            s->bis_rx_candidate_seen[phase]=skipped;
        }
        memset(s->bis_rx_candidate_eq,0,sizeof s->bis_rx_candidate_eq);
        s->bis_rx_candidate_active=1;s->bis_rx_selected_phase=-1;
        s->bis_rx_selected_alignment=0;
        s->bis_rx_selected_previous=s->bis_data.rx_previous&3u;
        s->rx_data_ready=1;
        return;
    }
    if (s->data_ready) return;
    if(s->startup.bis_selected){
        (void)v32bis_data_init(&s->bis_data,s->role,s->startup.selected_rate);
        (void)v32bis_qam_init(&s->bis_qam,s->startup.selected_rate);
        if(s->standard_startup){
            v32bis_qam_set_pulse_shaped(&s->bis_qam,1);
            s->bis_data.tx_scr=s->e_tx.scr;
            s->bis_data.rx_descr=s->e_rx.descr;
            s->bis_data.tx_previous=carrier_pair(s->last_tx);
            if(s->startup_scanner_selected>=0){
                const struct v32_startup_scanner *scanner=
                    &s->startup_scanner[s->startup_scanner_selected];
                const struct v32_line *rx=&scanner->line;
                s->bis_data.rx_previous=carrier_pair(scanner->last);
                s->bis_qam.rx_samples=rx->rx_samples;
            }
            s->bis_qam.tx_samples=s->line.tx_samples;
        }
    }
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
    s->rx_data_ready = 1;
    s->data_ready = 1;
}

static void generate_standard_bis(struct v32_session *s,int16_t *pcm,
                                  size_t count)
{
    if(!s->bis_qam_ready){
        (void)v32bis_qam_init(&s->bis_qam,s->startup.selected_rate);
        v32bis_qam_set_pulse_shaped(&s->bis_qam,1);
        s->bis_qam.tx_samples=s->line.tx_samples;
        s->bis_qam.shaped_loaded=(uint64_t)floor(
            (double)s->bis_qam.tx_samples*2400.0/8000.0);
        s->bis_qam_ready=1;
    }
    size_t symbols=count*2400/8000;
    for(size_t i=0;i<symbols;i++){
        if(!s->remote_r3){
            if(s->startup_rate_symbols>=V32_R3_RETRAIN_SYMBOLS)
                v32_retrain_request(&s->retrain);
            uint8_t state=(uint8_t)v32_rate_tx_next(&s->rate_tx);
            (void)v32bis_qam_write_carriers(&s->bis_qam,&state,1);
            s->last_tx=(enum v32_carrier_state)state;
            s->startup_rate_symbols++;
            continue;
        }
        if(!s->local_e_complete){
            if(!s->e_tx_ready&&(s->startup_rate_symbols&7u)!=0){
                uint8_t state=(uint8_t)v32_rate_tx_next(&s->rate_tx);
                (void)v32bis_qam_write_carriers(&s->bis_qam,&state,1);
                s->last_tx=(enum v32_carrier_state)state;
                s->startup_rate_symbols++;
                continue;
            }
            if(!s->e_tx_ready){
                v32_rate_tx_continue(&s->e_tx,&s->rate_tx,
                    v32bis_e_word(s->startup.selected_rate,1));
                s->e_tx_ready=1;s->startup.phase=V32_START_E;
            }
            uint8_t state=(uint8_t)v32_rate_tx_next(&s->e_tx);
            (void)v32bis_qam_write_carriers(&s->bis_qam,&state,1);
            s->last_tx=(enum v32_carrier_state)state;
            if(++s->local_e_symbols==8){
                s->local_e_complete=1;
                (void)v32bis_data_init(&s->bis_data,s->role,
                                       s->startup.selected_rate);
                s->bis_data.tx_scr=s->e_tx.scr;
                s->bis_data.tx_previous=carrier_pair(s->last_tx);
                s->data_ready=1;
                begin_data(s);
            }
            continue;
        }
        uint8_t label=(uint8_t)v32bis_data_next(&s->bis_data);
        (void)v32bis_qam_write(&s->bis_qam,&label,1);
        if(s->tx_marking<128)s->tx_marking++;
    }
    v32bis_qam_generate(&s->bis_qam,pcm,count);
    s->tx_samples+=count;
}

static int equalize_bis_point(struct v32bis_rx_equalizer *eq,
                              struct v32bis_sample observed,
                              struct v32bis_sample desired,int known,int rate,
                              struct v32bis_sample *result)
{
    enum{TAPS=9,DELAY=0};
    struct v32bis_sample raw=observed;
    if(eq->carrier_enabled){
        double c=cos(eq->carrier_phase),s=sin(eq->carrier_phase);
        observed.i=raw.i*c+raw.q*s;
        observed.q=raw.q*c-raw.i*s;
        eq->carrier_phase+=eq->carrier_step;
    }
    eq->history_at=(eq->history_at+1u)%TAPS;
    eq->history[eq->history_at]=observed;
    if(eq->history_count<TAPS)eq->history_count++;
    if(!eq->ready){
        unsigned at=(eq->history_at+TAPS-DELAY)%TAPS;
        struct v32bis_sample input=eq->history[at];
        double d=input.i*input.i+input.q*input.q;
        if(d<1e-9)return 0;
        eq->weights[DELAY]=(struct v32bis_sample){
            (desired.i*input.i+desired.q*input.q)/d,
            (desired.q*input.i-desired.i*input.q)/d};
        eq->ready=1;
    }
    struct v32bis_sample output={0,0};double input_power=1e-9;
    for(unsigned tap=0;tap<TAPS;tap++){
        unsigned at=(eq->history_at+TAPS-tap)%TAPS;
        struct v32bis_sample input=eq->history[at];
        struct v32bis_sample weight=eq->weights[tap];
        output.i+=weight.i*input.i-weight.q*input.q;
        output.q+=weight.i*input.q+weight.q*input.i;
        input_power+=input.i*input.i+input.q*input.q;
    }
    if(!known){
        unsigned label;double distance;
        if(v32bis_map_nearest(rate,output.i,output.q,&label,&distance)<0)return 0;
        struct v32bis_point decision;
        (void)v32bis_map_point(rate,label,&decision);
        double power=decision.i*decision.i+decision.q*decision.q;
        if(power<1e-9||distance>0.49*power){*result=output;return 1;}
        desired=(struct v32bis_sample){decision.i,decision.q};
    }
    if(known&&!eq->carrier_enabled&&eq->history_count==TAPS){
        double cross_i=output.i*desired.i+output.q*desired.q;
        double cross_q=output.q*desired.i-output.i*desired.q;
        if(eq->phase_count){
            double ci=cross_i*eq->carrier_cross_i+
                      cross_q*eq->carrier_cross_q;
            double cq=cross_q*eq->carrier_cross_i-
                      cross_i*eq->carrier_cross_q;
            double magnitude=hypot(ci,cq);
            if(magnitude>1e-9){
                eq->carrier_correlation_i+=ci/magnitude;
                eq->carrier_correlation_q+=cq/magnitude;
            }
        }
        eq->carrier_cross_i=cross_i;eq->carrier_cross_q=cross_q;
        eq->phase_count++;
    }
    double er=desired.i-output.i,error_q=desired.q-output.q;
    if(known&&eq->history_count==TAPS){
        eq->error+=er*er+error_q*error_q;
        eq->power+=desired.i*desired.i+desired.q*desired.q;
        eq->observed_power+=raw.i*raw.i+raw.q*raw.q;
        eq->known_points++;
    }
    double mu=known?0.45:0.008;
    for(unsigned tap=0;tap<TAPS;tap++){
        unsigned at=(eq->history_at+TAPS-tap)%TAPS;
        struct v32bis_sample input=eq->history[at];
        eq->weights[tap].i+=mu*(er*input.i+error_q*input.q)/input_power;
        eq->weights[tap].q+=mu*(error_q*input.i-er*input.q)/input_power;
    }
    eq->h_re=eq->weights[DELAY].i;eq->h_im=eq->weights[DELAY].q;
    *result=output;
    return 1;
}

static void receive_bis_points(struct v32_session *s)
{
    struct v32bis_sample points[128];
    size_t n=v32bis_qam_read(&s->bis_qam,points,128);
    for(size_t i=0;i<n;i++){
        int known=s->rx_marking<128;
        struct v32bis_sample desired=known?s->bis_rx_expected[s->rx_marking]:
                                              (struct v32bis_sample){0,0};
        struct v32bis_sample output=points[i];
        if(!equalize_bis_point(&s->bis_rx_eq,points[i],desired,known,
                               s->startup.selected_rate,&output))continue;
        if(known)output=desired;
        (void)v32bis_data_put(&s->bis_data,output.i,output.q);
        if(known&&++s->rx_marking==128)s->bis_data.rh=s->bis_data.rt;
    }
    s->bis_rx_known=s->rx_marking<128?128-s->rx_marking:0;
}

static void receive_bis_timing_candidates(struct v32_session *s,
                                           const int16_t *pcm,size_t count)
{
    int complete=1;
    for(unsigned phase=0;phase<V32BIS_RX_TIMING_PHASES;phase++){
        struct v32bis_qam *qam=&s->bis_rx_candidate_qam[phase];
        v32bis_qam_receive(qam,pcm,count);
        unsigned seen=s->bis_rx_candidate_seen[phase];
        unsigned remaining=seen<s->bis_rx_candidate_target?
                           s->bis_rx_candidate_target-seen:0;
        struct v32bis_sample points[128];
        size_t n=v32bis_qam_read(qam,points,remaining);
        for(size_t i=0;i<n;i++){
            for(unsigned alignment=0;alignment<V32BIS_RX_ALIGNMENTS;
                alignment++){
                int expected=(int)seen+(int)alignment-
                             V32BIS_RX_ALIGNMENT_RADIUS;
                if(expected<0||expected>=128)continue;
                for(unsigned previous=0;
                    previous<V32BIS_RX_DIFFERENTIAL_STATES;previous++){
                    unsigned candidate=
                        (alignment*V32BIS_RX_DIFFERENTIAL_STATES+previous)*
                        V32BIS_RX_TIMING_PHASES+phase;
                    struct v32bis_sample output;
                    (void)equalize_bis_point(
                        &s->bis_rx_candidate_eq[candidate],points[i],
                        s->bis_rx_candidate_expected[previous][expected],1,
                        s->startup.selected_rate,&output);
                }
            }
            seen++;
        }
        s->bis_rx_candidate_seen[phase]=seen;
        if(seen<s->bis_rx_candidate_target)complete=0;
    }
    if(!complete)return;
    unsigned best=0;double best_score=HUGE_VAL;
    for(unsigned candidate=0;candidate<V32BIS_RX_CANDIDATES;candidate++){
        const struct v32bis_rx_equalizer *eq=
            &s->bis_rx_candidate_eq[candidate];
        double score=eq->power>0.0?eq->error/eq->power:HUGE_VAL;
        if(score<best_score){best=candidate;best_score=score;}
    }
    unsigned best_phase=best%V32BIS_RX_TIMING_PHASES;
    unsigned pair=best/V32BIS_RX_TIMING_PHASES;
    unsigned best_previous=pair%V32BIS_RX_DIFFERENTIAL_STATES;
    unsigned best_alignment=pair/V32BIS_RX_DIFFERENTIAL_STATES;
    int alignment=(int)best_alignment-V32BIS_RX_ALIGNMENT_RADIUS;
    v32bis_qam_copy_receiver(&s->bis_qam,
                             &s->bis_rx_candidate_qam[best_phase]);
    s->bis_rx_eq=s->bis_rx_candidate_eq[best];
    s->bis_rx_selected_phase=(int)best_phase;
    s->bis_rx_selected_previous=best_previous;
    s->bis_rx_selected_alignment=alignment;
    s->bis_rx_acquisition_complete=1;
    s->bis_rx_acquisition_ok=
        best_score<=V32BIS_RX_MAX_B1_EVM*V32BIS_RX_MAX_B1_EVM;
    if(!s->bis_rx_acquisition_ok){
        s->bis_rx_candidate_active=0;
        s->bis_rx_retrain_ab_at_failure=s->retrain.ab_count;
        return;
    }
    {
        struct v32bis_rx_equalizer *eq=&s->bis_rx_eq;
        double correlations=eq->phase_count>1?(double)(eq->phase_count-1):1.0;
        eq->carrier_confidence=hypot(eq->carrier_correlation_i,
                                     eq->carrier_correlation_q)/correlations;
        eq->carrier_step=atan2(eq->carrier_correlation_q,
                               eq->carrier_correlation_i);
        if(eq->carrier_confidence<0.60||
           fabs(eq->carrier_step)>2.0*M_PI*7.0/2400.0)
            eq->carrier_step=0.0;
        eq->carrier_phase=0.0;eq->carrier_enabled=0;
    }
    memcpy(s->bis_rx_expected,s->bis_rx_candidate_expected[best_previous],
           sizeof s->bis_rx_expected);
    s->bis_data.rx_previous=best_previous;
    int consumed=(int)s->bis_rx_candidate_target+alignment;
    if(consumed<0)consumed=0;
    if(consumed>128)consumed=128;
    for(int k=0;k<consumed;k++){
        struct v32bis_sample point=s->bis_rx_expected[k];
        (void)v32bis_data_put(&s->bis_data,point.i,point.q);
    }
    s->rx_marking=(unsigned)consumed;
    s->bis_rx_known=128-s->rx_marking;
    s->bis_rx_candidate_active=0;
    receive_bis_points(s);
}

void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count)
{
    if (s->standard_startup && s->retrain.state != V32_RETRAIN_IDLE) {
        unsigned rates=s->startup.allowed_rates;
        standard_retrain_begin(s,rates);
    }
    if (s->retrain.state == V32_RETRAIN_RESTART) {
        unsigned rates=s->startup.allowed_rates;
        media_init(s,s->role,rates);
    }
    if (s->retrain.state == V32_RETRAIN_REQUEST ||
        s->retrain.state == V32_RETRAIN_ACK) {
        enum v32_carrier_state states[64]; size_t n=count*2400/8000;
        for(size_t i=0;i<n;i++)states[i]=v32_retrain_next(&s->retrain);
        (void)v32_line_write(&s->line,states,n);
        v32_line_generate(&s->line,pcm,count);
        s->tx_samples+=count;
        return;
    }
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
        if(s->startup.phase==V32_START_RATE_2&&s->startup.bis_selected){
            generate_standard_bis(s,pcm,count);return;
        }
        queue_standard_training(s,count*2400/8000);
        v32_line_generate(&s->line,pcm,count);s->tx_samples+=count;return;
    }
    if(s->standard_startup&&s->startup.bis_selected&&s->bis_qam_ready){
        generate_standard_bis(s,pcm,count);return;
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
    int waiting_e=s->standard_startup&&s->startup.phase==V32_START_E&&
                  !s->remote_e;
    int waiting_r3=s->standard_startup&&
                   (s->startup.phase==V32_START_TRAIN_2||
                    s->startup.phase==V32_START_RATE_2)&&!s->remote_r3;
    if(!v32_session_connected(s)&&s->retrain.state==V32_RETRAIN_IDLE&&
       !s->bis_rx_candidate_active&&!waiting_e&&!waiting_r3&&
       !(s->bis_rx_acquisition_complete&&!s->bis_rx_acquisition_ok))return 0;
    if(s->standard_startup){
        const double frequencies[3]={600.0,1800.0,3000.0};
        unsigned first=s->role==V32_STD_CALL?0:1;
        unsigned last=s->role==V32_STD_CALL?2:1;
        double energy=0.0,best=0.0;
        for(size_t k=0;k<count;k++){double x=pcm[k];energy+=x*x;}
        for(unsigned f=first;f<=last;f+=(last==first?1:2)){
            double i=0.0,q=0.0;
            for(size_t k=0;k<count;k++){
                double p=2.0*M_PI*frequencies[f]*(double)k/8000.0;
                i+=(double)pcm[k]*cos(p);q-=(double)pcm[k]*sin(p);
            }
            double power=i*i+q*q;if(power>best)best=power;
            if(last==first)break;
        }
        if(energy>=1.0&&best>=0.10*energy*(double)count){
            if(++s->retrain_tone_blocks>=3){
                unsigned rates=s->startup.allowed_rates;
                standard_retrain_begin(s,rates);
                return 1;
            }
        }else s->retrain_tone_blocks=0;
        return s->retrain.state!=V32_RETRAIN_IDLE;
    }
    enum v32_carrier_state states[128];v32_line_receive(&s->retrain_monitor,pcm,count);
    size_t n;while((n=v32_line_read(&s->retrain_monitor,states,128))!=0)
        for(size_t i=0;i<n;i++)if(v32_retrain_put(&s->retrain,states[i])){
            /* RESTART is consumed by v32_session_generate().  In particular,
             * standard sessions must enter their second S/Sbar/TRN interval;
             * resetting media here would lose that state and fall back to R1. */
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
                        v32_e_rx_expect(&s->e_rx,v32bis_e_word(
                            s->startup.selected_rate,!!s->startup.bis_selected));
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
        if(monitor_retrain(s,pcm,count)){s->rx_samples+=count;return;}
        startup_r3_receive(s,pcm,count);s->rx_samples+=count;return;
    }
    if(s->standard_startup&&s->startup.phase==V32_START_E&&
       !s->remote_e&&s->startup_scanner_selected>=0){
        if(monitor_retrain(s,pcm,count)){s->rx_samples+=count;return;}
        startup_r3_receive(s,pcm,count);
        if(!s->remote_e&&s->e_rx.words>=V32_E_RETRAIN_WORDS)
            v32_retrain_request(&s->retrain);
        s->rx_samples+=count;return;
    }
    if(monitor_retrain(s,pcm,count)){s->rx_samples+=count;return;}
    if(s->standard_startup&&s->bis_rx_acquisition_complete&&
       !s->bis_rx_acquisition_ok){
        s->bis_rx_reject_wait_symbols+=(unsigned)(count*2400/8000);
        if(s->bis_rx_reject_wait_symbols>=V32BIS_RX_RETRAIN_WAIT_SYMBOLS)
            v32_retrain_request(&s->retrain);
        s->rx_samples+=count;return;
    }
    begin_data(s);
    if(s->rx_data_ready&&s->startup.bis_selected){
        if(s->standard_startup&&s->bis_rx_candidate_active)
            receive_bis_timing_candidates(s,pcm,count);
        else{
            v32bis_qam_receive(&s->bis_qam,pcm,count);
            if(s->standard_startup)receive_bis_points(s);
            else{
                struct v32bis_sample points[128];
                size_t n=v32bis_qam_read(&s->bis_qam,points,128);
                for(size_t i=0;i<n;i++){
                    (void)v32bis_data_put(&s->bis_data,points[i].i,points[i].q);
                    if(s->rx_marking<128&&++s->rx_marking==128)
                        s->bis_data.rh=s->bis_data.rt;
                }
            }
        }
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
{ return (s->startup.bis_selected?s->rx_data_ready:s->data_ready) ? (s->startup.bis_selected?v32bis_data_read(&s->bis_data,b,n):v32_data_read(&s->data, b, n)) : 0; }
int v32_session_connected(const struct v32_session *s)
{ return s->retrain.state==V32_RETRAIN_IDLE && s->data_ready && s->tx_marking >= 128 && s->rx_marking >= 128; }
int v32_session_rate(const struct v32_session *s) { return s->startup.selected_rate; }
size_t v32_session_pending(const struct v32_session*s)
{return(s->pending_tail+sizeof s->pending-s->pending_head)%sizeof s->pending;}
