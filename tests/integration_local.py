#!/usr/bin/env python3
"""Local two-process SIP/RTP/PTY smoke test; uses no external network."""
import os
import select
import subprocess
import tempfile
import termios
import time


def read_until(fd, marker, timeout=12):
    data = bytearray()
    deadline = time.monotonic() + timeout
    while marker not in data and time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], 0.2)
        if ready:
            data.extend(os.read(fd, 4096))
    return bytes(data)


def raw_open(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] |= termios.CLOCAL | termios.CREAD
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def main():
    with tempfile.TemporaryDirectory(prefix="softmodem-test-") as root:
        paths = [os.path.join(root, "a"), os.path.join(root, "b")]
        procs = []
        for index, (sip, rtp, peer) in enumerate(((15060, 11000, 15061), (15061, 11002, 15060))):
            env = os.environ.copy()
            env.update(SOFTMODEM_BIND_IP="127.0.0.1", SOFTMODEM_PUBLIC_IP="127.0.0.1",
                       SOFTMODEM_SIP_PORT=str(sip), SOFTMODEM_RTP_PORT=str(rtp),
                       SOFTMODEM_OUTBOUND_HOST="127.0.0.1", SOFTMODEM_OUTBOUND_PORT=str(peer),
                       SOFTMODEM_TTY=paths[index], SOFTMODEM_PROTOCOLS="V22BIS",
                       SOFTMODEM_MAX_RATE="2400")
            procs.append(subprocess.Popen(["./sip-softmodem"], env=env,
                                          stdout=subprocess.DEVNULL, stderr=subprocess.PIPE))
        fds = []
        try:
            deadline = time.monotonic() + 3
            while not all(os.path.exists(path) for path in paths) and time.monotonic() < deadline:
                time.sleep(0.05)
            for proc in procs:
                if proc.poll() is not None:
                    raise RuntimeError("softmodem startup failed: " + proc.stderr.read().decode(errors="replace"))
            fds = [raw_open(path) for path in paths]
            os.write(fds[0], b"ATDT123\r")
            result = read_until(fds[0], b"CONNECT 2400")
            if b"CONNECT 2400" not in result:
                raise RuntimeError("caller did not connect: " + repr(result))
            payload = b"hello-over-v22bis\r\n"
            os.write(fds[0], payload)
            received = read_until(fds[1], payload, 5)
            if payload not in received:
                raise RuntimeError("payload mismatch: " + repr(received))
            print("local SIP/RTP/PTY integration: CONNECT 2400, payload received exactly")
        finally:
            for fd in fds:
                os.close(fd)
            for proc in procs:
                proc.terminate()
            for proc in procs:
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()


if __name__ == "__main__":
    main()
