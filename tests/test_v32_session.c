#include "pcma.h"
#include "v32_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void block(struct v32_session *a, struct v32_session *b)
{
    int16_t x[160], y[160], dx[160], dy[160]; uint8_t ax[160], ay[160];
    v32_session_generate(a, x, 160); v32_session_generate(b, y, 160);
    pcma_encode_buffer(x, ax, 160); pcma_encode_buffer(y, ay, 160);
    pcma_decode_buffer(ax, dx, 160); pcma_decode_buffer(ay, dy, 160);
    v32_session_receive(a, dy, 160); v32_session_receive(b, dx, 160);
}

static void run(int allow_9600, int expected)
{
    struct v32_session a, b; v32_session_init(&a, V32_STD_CALL, 1, allow_9600);
    v32_session_init(&b, V32_STD_ANSWER, 1, allow_9600);
    for (int i = 0; i < 80; i++) block(&a, &b);
    assert(v32_session_connected(&a) && v32_session_connected(&b));
    assert(v32_session_rate(&a) == expected && v32_session_rate(&b) == expected);
    static const uint8_t msg[] = "full-v32-session"; uint8_t got[64] = {0};
    assert(v32_session_write(&a, msg, sizeof msg) == sizeof msg);
    for (int i = 0; i < 30; i++) block(&a, &b);
    size_t n = v32_session_read(&b, got, sizeof got);
    if (n < sizeof msg || memcmp(msg, got, sizeof msg))
        { fprintf(stderr, "V.32 %d payload mismatch: got %zu bytes:", expected, n);
          for (size_t i = 0; i < n; i++) fprintf(stderr, " %02x", got[i]);
          fputc('\n', stderr); }
    assert(n >= sizeof msg && !memcmp(msg, got, sizeof msg));
    v32_session_media_gap(&a);
    assert(!v32_session_connected(&a));
    static const uint8_t queued[]="queued-during-retrain";
    assert(v32_session_write(&a,queued,sizeof queued)==sizeof queued);
    assert(v32_session_pending(&a)==sizeof queued);
    for(int i=0;i<100&&(!v32_session_connected(&a)||!v32_session_connected(&b));i++)block(&a,&b);
    assert(v32_session_connected(&a)&&v32_session_connected(&b));
    for(int i=0;i<30;i++)block(&a,&b);
    memset(got,0,sizeof got);n=v32_session_read(&b,got,sizeof got);
    assert(n>=sizeof queued&&!memcmp(queued,got,sizeof queued));
    assert(v32_session_pending(&a)==0);
    memset(got,0,sizeof got);assert(v32_session_write(&b,msg,sizeof msg)==sizeof msg);
    for(int i=0;i<30;i++)block(&a,&b);
    n=v32_session_read(&a,got,sizeof got);
    assert(n>=sizeof msg&&!memcmp(msg,got,sizeof msg));
    printf("V.32 composite PCMA session: CONNECT %d, retrain and exact payload\n", v32_session_rate(&a));
}

static void run_bis(int rate)
{
    struct v32_session a,b;v32bis_session_init(&a,V32_STD_CALL,rate);v32bis_session_init(&b,V32_STD_ANSWER,rate);
    for(int i=0;i<100&&!v32_session_connected(&a);i++)block(&a,&b);
    assert(v32_session_connected(&a)&&v32_session_connected(&b));assert(v32_session_rate(&a)==rate&&v32_session_rate(&b)==rate);
    uint8_t source[256],got[300]={0};for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*41u+5u);assert(v32_session_write(&a,source,sizeof source)==sizeof source);
    for(int i=0;i<100;i++)block(&a,&b);
    size_t n=v32_session_read(&b,got,sizeof got);assert(n>=sizeof source&&!memcmp(source,got,sizeof source));
    printf("V.32bis composite PCMA session: CONNECT %d and exact payload\n",rate);
}

static void standard_retrain_preempts_bis_data(void)
{
    struct v32_session s;
    int16_t pcm[160];
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.startup.phase=V32_START_ONES_128;
    s.startup.bis_selected=1;
    s.bis_qam_ready=1;
    v32_retrain_request(&s.retrain);
    v32_session_generate(&s,pcm,160);
    assert(s.retrain.state==V32_RETRAIN_REQUEST);
    assert(s.retrain.tx_count==48);
}

static void rejected_b1_retrain_arbitration(int remote_request)
{
    struct v32_session s;
    struct v32_line tx;
    struct v32_retrain remote;
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.startup.phase=V32_START_ONES_128;
    s.startup.selected_rate=9600;
    s.startup.bis_selected=1;
    s.remote_e=s.data_ready=s.rx_data_ready=1;
    s.bis_rx_acquisition_complete=1;
    s.bis_rx_acquisition_ok=0;
    v32_line_init(&tx);
    v32_retrain_init(&remote);
    if(remote_request)v32_retrain_request(&remote);
    for(unsigned block_index=0;block_index<12&&
        s.retrain.state==V32_RETRAIN_IDLE;block_index++){
        enum v32_carrier_state states[48];
        for(unsigned n=0;n<48;n++)
            states[n]=remote_request?v32_retrain_next(&remote):V32_STATE_A;
        assert(v32_line_write(&tx,states,48)==48);
        int16_t pcm[160],decoded[160];uint8_t law[160];
        v32_line_generate(&tx,pcm,160);
        pcma_encode_buffer(pcm,law,160);
        pcma_decode_buffer(law,decoded,160);
        v32_session_receive(&s,decoded,160);
    }
    assert(s.retrain.state==
           (remote_request?V32_RETRAIN_ACK:V32_RETRAIN_REQUEST));
}

static void waiting_e_retrain_arbitration(int remote_request)
{
    struct v32_session s;
    struct v32_line tx;
    struct v32_retrain remote;
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.startup.phase=V32_START_E;
    s.startup.selected_rate=9600;
    s.startup.bis_selected=1;
    s.startup_scanner_selected=0;
    s.e_rx_ready=1;
    v32_line_init(&tx);
    v32_retrain_init(&remote);
    if(remote_request)v32_retrain_request(&remote);
    if(!remote_request)s.e_rx.words=V32_E_RETRAIN_WORDS;
    for(unsigned block_index=0;block_index<12&&
        s.retrain.state==V32_RETRAIN_IDLE;block_index++){
        enum v32_carrier_state states[48];
        for(unsigned n=0;n<48;n++)
            states[n]=remote_request?v32_retrain_next(&remote):V32_STATE_A;
        assert(v32_line_write(&tx,states,48)==48);
        int16_t pcm[160],decoded[160];uint8_t law[160];
        v32_line_generate(&tx,pcm,160);
        pcma_encode_buffer(pcm,law,160);
        pcma_decode_buffer(law,decoded,160);
        v32_session_receive(&s,decoded,160);
    }
    assert(s.retrain.state==
           (remote_request?V32_RETRAIN_ACK:V32_RETRAIN_REQUEST));
}

static void waiting_r3_remote_retrain(void)
{
    struct v32_session s;
    struct v32_line tx;
    struct v32_retrain remote;
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.startup.phase=V32_START_RATE_2;
    s.startup.selected_rate=9600;
    s.startup.bis_selected=1;
    s.startup_scanner_selected=0;
    v32_line_init(&tx);
    v32_retrain_init(&remote);
    v32_retrain_request(&remote);
    for(unsigned block_index=0;block_index<12&&
        s.retrain.state==V32_RETRAIN_IDLE;block_index++){
        enum v32_carrier_state states[48];
        for(unsigned n=0;n<48;n++)states[n]=v32_retrain_next(&remote);
        assert(v32_line_write(&tx,states,48)==48);
        int16_t pcm[160],decoded[160];uint8_t law[160];
        v32_line_generate(&tx,pcm,160);
        pcma_encode_buffer(pcm,law,160);
        pcma_decode_buffer(law,decoded,160);
        v32_session_receive(&s,decoded,160);
    }
    assert(s.retrain.state==V32_RETRAIN_ACK);
}

static void waiting_r3_local_timeout(void)
{
    struct v32_session s;int16_t pcm[160];
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.startup.phase=V32_START_RATE_2;
    s.startup.selected_rate=9600;
    s.startup.bis_selected=1;
    s.startup.local_rate_word=v32bis_rate_word(V32_RATE_9600,1);
    v32_rate_tx_init(&s.rate_tx,V32_STD_CALL,s.startup.local_rate_word,
                     V32_STATE_A);
    s.rate_tx_ready=1;
    s.startup_rate_symbols=V32_R3_RETRAIN_SYMBOLS;
    v32_session_generate(&s,pcm,160);
    assert(s.retrain.state==V32_RETRAIN_REQUEST);
}

static void standard_retrain_restarts_second_training(void)
{
    struct v32_session s;int16_t pcm[160];
    v32bis_session_init(&s,V32_STD_CALL,9600);
    v32_session_start_standard(&s);
    s.retrain.state=V32_RETRAIN_RESTART;
    v32_session_generate(&s,pcm,160);
    assert(s.retrain.state==V32_RETRAIN_IDLE);
    assert(s.startup.phase==V32_START_TRAIN_2);
    assert(s.startup_training_symbols==48);
    long energy=0;
    for(unsigned n=0;n<160;n++)energy+=pcm[n]>=0?pcm[n]:-(long)pcm[n];
    assert(energy>0);
}

int main(void)
{
    run(0, 4800);
    run(1, 9600);
    run_bis(7200);run_bis(9600);run_bis(12000);run_bis(14400);
    standard_retrain_preempts_bis_data();
    rejected_b1_retrain_arbitration(0);
    rejected_b1_retrain_arbitration(1);
    waiting_e_retrain_arbitration(0);
    waiting_e_retrain_arbitration(1);
    waiting_r3_remote_retrain();
    waiting_r3_local_timeout();
    standard_retrain_restarts_second_training();
    return 0;
}
