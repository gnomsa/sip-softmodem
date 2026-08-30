#!/bin/sh
set -eu

work=$(mktemp -d)
pid=
cleanup()
{
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        kill -KILL "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rmdir "$work" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

sip_port=$((30000 + $$ % 5000))
rtp_port=$((40000 + $$ % 5000))
SOFTMODEM_BIND_IP=127.0.0.1 \
SOFTMODEM_PUBLIC_IP=127.0.0.1 \
SOFTMODEM_SIP_PORT=$sip_port \
SOFTMODEM_RTP_PORT=$rtp_port \
SOFTMODEM_CHANNELS=4 \
SOFTMODEM_PROTOCOLS=V32BIS \
SOFTMODEM_MAX_RATE=9600 \
SOFTMODEM_TTY="$work/ttyMODEM0" \
./sip-softmodem >"$work/log" 2>&1 &
pid=$!

i=0
while [ "$i" -lt 30 ] && [ ! -L "$work/ttyMODEM3" ]; do
    if ! kill -0 "$pid" 2>/dev/null; then
        wait "$pid" || true
        sed -n '1,80p' "$work/log"
        exit 1
    fi
    sleep 0.1
    i=$((i + 1))
done
test -L "$work/ttyMODEM3"

kill -TERM "$pid"
i=0
while [ "$i" -lt 30 ] && kill -0 "$pid" 2>/dev/null; do
    sleep 0.1
    i=$((i + 1))
done
if kill -0 "$pid" 2>/dev/null; then
    echo "softmodem did not stop within 3 seconds" >&2
    exit 1
fi
wait "$pid"
pid=
rm "$work/log"
rmdir "$work"
trap - EXIT INT TERM
echo "four-channel SIGTERM shutdown passed"
