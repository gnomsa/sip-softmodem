#include "v32_session.h"

static enum v32_std_role other(enum v32_std_role r)
{
    return r == V32_STD_CALL ? V32_STD_ANSWER : V32_STD_CALL;
}

static void media_init(struct v32_session *s,enum v32_std_role role,
                       unsigned rates)
{
    s->role=role;s->last_tx=s->last_rx=V32_STATE_A;
    s->tx_samples=s->rx_samples=0;s->tx_symbols=s->rx_symbols=0;
    s->tx_marking=s->rx_marking=0;
    s->rate_tx_ready=s->rate_rx_ready=s->e_tx_ready=s->e_rx_ready=0;
    s->remote_e=s->data_ready=0;
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

static void begin_data(struct v32_session *s)
{
    if (s->data_ready || !s->startup.selected_rate || !s->remote_e) return;
    /* E is the common differential reference at the data boundary.  The
     * composite session currently represents that boundary as state A. */
    if(s->startup.bis_selected){(void)v32bis_data_init(&s->bis_data,s->role,s->startup.selected_rate);(void)v32bis_qam_init(&s->bis_qam,s->startup.selected_rate);}
    else{v32_data_init(&s->data, s->role, V32_STATE_A, V32_STATE_A);v32_qam_init(&s->qam);}
    s->data_ready = 1;
}

void v32_session_generate(struct v32_session *s, int16_t *pcm, size_t count)
{
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
