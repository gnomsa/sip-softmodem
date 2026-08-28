#include "v42_v32.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    struct v42_v32 a,b;v42_v32_init(&a,V32_STD_CALL,1,1);v42_v32_init(&b,V32_STD_ANSWER,1,1);
    uint8_t source[1000],got[1000];for(size_t i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*29u+3u);
    assert(v42_v32_write(&a,source,sizeof source)==sizeof source);size_t received=0;unsigned blocks;
    for(blocks=0;blocks<4000&&received<sizeof got;blocks++){
        int16_t ab[160],ba[160];v42_v32_generate(&a,ab,160);v42_v32_generate(&b,ba,160);
        v42_v32_receive(&b,ab,160);v42_v32_receive(&a,ba,160);
        received+=v42_v32_read(&b,got+received,sizeof got-received);
    }
    assert(v42_v32_connected(&a)&&v42_v32_connected(&b));
    assert(received==sizeof source&&!memcmp(source,got,sizeof source));
    double seconds=blocks*0.02;printf("V.42 over V.32/PCMA: %zu bytes exact at %d bit/s in %.2f s (%.1f B/s application)\n",received,v42_v32_rate(&a),seconds,received/seconds);return 0;
}
