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

int main(void)
{
    struct v32_session a, b; v32_session_init(&a, V32_STD_CALL, 1, 1);
    v32_session_init(&b, V32_STD_ANSWER, 1, 1);
    for (int i = 0; i < 80; i++) block(&a, &b);
    assert(v32_session_connected(&a) && v32_session_connected(&b));
    assert(v32_session_rate(&a) == 9600 && v32_session_rate(&b) == 9600);
    static const uint8_t msg[] = "full-v32-session"; uint8_t got[64] = {0};
    assert(v32_session_write(&a, msg, sizeof msg) == sizeof msg);
    for (int i = 0; i < 30; i++) block(&a, &b);
    size_t n = v32_session_read(&b, got, sizeof got);
    assert(n >= sizeof msg && !memcmp(msg, got, sizeof msg));
    printf("V.32 composite PCMA session: CONNECT %d, exact payload\n", v32_session_rate(&a));
    return 0;
}
