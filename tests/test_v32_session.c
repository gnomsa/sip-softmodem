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
    printf("V.32 composite PCMA session: CONNECT %d, E + 128 marking + exact payload\n", v32_session_rate(&a));
}

int main(void)
{
    run(0, 4800);
    run(1, 9600);
    return 0;
}
