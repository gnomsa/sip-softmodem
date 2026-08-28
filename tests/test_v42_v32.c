#include "v42_v32.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void run(int rate)
{
    struct v42_v32 a,b;v42_v32_init_rate(&a,V32_STD_CALL,rate);v42_v32_init_rate(&b,V32_STD_ANSWER,rate);
    uint8_t source[1000],got[1000];for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*29u+3u);
    assert(v42_v32_write(&a,source,sizeof source)==sizeof source);size_t received=0;unsigned blocks;
    for(blocks=0;blocks<4000&&received<sizeof got;blocks++){
        int16_t ab[160],ba[160];v42_v32_generate(&a,ab,160);v42_v32_generate(&b,ba,160);
        v42_v32_receive(&b,ab,160);v42_v32_receive(&a,ba,160);
        received+=v42_v32_read(&b,got+received,sizeof got-received);
    }
    assert(v42_v32_connected(&a)&&v42_v32_connected(&b));
    assert(received==sizeof source&&!memcmp(source,got,sizeof source));
    double seconds=blocks*0.02;printf("V.42 over V.32bis/PCMA: %zu bytes exact at %d bit/s in %.2f s (%.1f B/s application)\n",received,v42_v32_rate(&a),seconds,received/seconds);
}
int main(void){run(4800);run(7200);run(9600);run(12000);run(14400);return 0;}
