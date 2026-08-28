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


def read_count(fd, count, timeout):
    data = bytearray()
    deadline = time.monotonic() + timeout
    while len(data) < count and time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], 0.2)
        if ready:
            data.extend(os.read(fd, count - len(data)))
    return bytes(data)


def write_all(fd, data, timeout=5):
    offset = 0
    deadline = time.monotonic() + timeout
    while offset < len(data) and time.monotonic() < deadline:
        _, ready, _ = select.select([], [fd], [], 0.2)
        if ready:
            offset += os.write(fd, data[offset:])
    if offset != len(data):
        raise RuntimeError(f"PTY write stalled after {offset}/{len(data)} bytes")


def raw_open(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] |= termios.CLOCAL | termios.CREAD
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def main():
    protocol = os.environ.get("SOFTMODEM_TEST_PROTOCOL", "V22BIS")
    rate = int(os.environ.get("SOFTMODEM_TEST_RATE", "2400"))
    payload_size = int(os.environ.get("SOFTMODEM_TEST_PAYLOAD_BYTES", "0"))
    marker = f"CONNECT {rate}".encode()
    with tempfile.TemporaryDirectory(prefix="softmodem-test-") as root:
        paths = [os.path.join(root, "a"), os.path.join(root, "b")]
        procs = []
        logs = []
        for index, (sip, rtp, peer) in enumerate(((15060, 11000, 15061), (15061, 11002, 15060))):
            env = os.environ.copy()
            env.update(SOFTMODEM_BIND_IP="127.0.0.1", SOFTMODEM_PUBLIC_IP="127.0.0.1",
                       SOFTMODEM_SIP_PORT=str(sip), SOFTMODEM_RTP_PORT=str(rtp),
                       SOFTMODEM_OUTBOUND_HOST="127.0.0.1", SOFTMODEM_OUTBOUND_PORT=str(peer),
                       SOFTMODEM_TTY=paths[index], SOFTMODEM_PROTOCOLS=protocol,
                       SOFTMODEM_MAX_RATE=str(rate))
            logs.append(tempfile.TemporaryFile())
            procs.append(subprocess.Popen(["./sip-softmodem"], env=env,
                                          stdout=subprocess.DEVNULL, stderr=logs[-1]))
        fds = []
        try:
            deadline = time.monotonic() + 3
            while not all(os.path.exists(path) for path in paths) and time.monotonic() < deadline:
                time.sleep(0.05)
            for proc in procs:
                if proc.poll() is not None:
                    raise RuntimeError("softmodem startup failed")
            fds = [raw_open(path) for path in paths]
            os.write(fds[0], b"ATDT123\r")
            result = read_until(fds[0], marker, 35)
            if marker not in result:
                details=[]
                for log in logs:
                    log.flush();log.seek(0);details.append(log.read().decode(errors="replace"))
                raise RuntimeError("caller did not connect: " + repr(result) + " logs=" + repr(details))
            answer_result = read_until(fds[1], marker, 5)
            if marker not in answer_result:
                raise RuntimeError("answerer did not connect: " + repr(answer_result))
            if payload_size:
                payload = bytes((index * 73 + index // 251 + 19) & 0xff
                                for index in range(payload_size))
            else:
                payload = b"hello-over-" + protocol.lower().encode() + b"\r\n"
            started = time.monotonic()
            write_all(fds[0], payload)
            timeout = max(5.0, len(payload) * 10.0 / rate * 3.0 + 3.0)
            received = read_count(fds[1], len(payload), timeout)
            elapsed = time.monotonic() - started
            if received != payload:
                raise RuntimeError("payload mismatch: " + repr(received))
            for log in logs:
                log.flush();log.seek(0)
                content = log.read().decode(errors="replace")
                if "V.8 selected" not in content:
                    raise RuntimeError("V.8 was not selected: " + content)
            throughput = len(payload) / elapsed
            line_use = throughput * 10.0 * 100.0 / rate
            print(f"local SIP/RTP/PTY integration: CONNECT {rate}, "
                  f"{len(payload)} exact bytes, {throughput:.1f} B/s payload, "
                  f"{line_use:.1f}% of the 8N1 line rate")
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
            for log in logs:
                log.close()


if __name__ == "__main__":
    main()
