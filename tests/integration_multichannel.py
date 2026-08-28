#!/usr/bin/env python3
"""Four simultaneous calls through one SIP socket and four RTP/PTY channels."""
import os
import subprocess
import tempfile
import time

from integration_local import raw_open, read_count, read_until, write_all


CHANNELS = 4
RATE = 33600
MARKER = b"CONNECT 33600"


def start(env, log):
    return subprocess.Popen(["./sip-softmodem"], env=env,
                            stdout=subprocess.DEVNULL, stderr=log)


def log_text(log):
    log.flush()
    log.seek(0)
    return log.read().decode(errors="replace")


def main():
    with tempfile.TemporaryDirectory(prefix="softmodem-multi-") as root:
        server_base = os.path.join(root, "server0")
        server_paths = [os.path.join(root, f"server{i}")
                        for i in range(CHANNELS)]
        caller_paths = [os.path.join(root, f"caller{i}")
                        for i in range(CHANNELS)]
        logs = [tempfile.TemporaryFile() for _ in range(CHANNELS + 1)]
        processes = []
        fds = []
        common = dict(SOFTMODEM_BIND_IP="127.0.0.1",
                      SOFTMODEM_PUBLIC_IP="127.0.0.1",
                      SOFTMODEM_PROTOCOLS="V34",
                      SOFTMODEM_MAX_RATE=str(RATE))
        try:
            server_env = os.environ.copy()
            server_env.update(common)
            server_env.update(SOFTMODEM_SIP_PORT="15060",
                              SOFTMODEM_RTP_PORT="11000",
                              SOFTMODEM_TTY=server_base,
                              SOFTMODEM_CHANNELS=str(CHANNELS))
            processes.append(start(server_env, logs[0]))
            for index in range(CHANNELS):
                env = os.environ.copy()
                env.update(common)
                env.update(SOFTMODEM_SIP_PORT=str(15061 + index),
                           SOFTMODEM_RTP_PORT=str(12000 + 2 * index),
                           SOFTMODEM_TTY=caller_paths[index],
                           SOFTMODEM_CHANNELS="1",
                           SOFTMODEM_OUTBOUND_HOST="127.0.0.1",
                           SOFTMODEM_OUTBOUND_PORT="15060")
                processes.append(start(env, logs[index + 1]))

            paths = server_paths + caller_paths
            deadline = time.monotonic() + 5
            while not all(os.path.exists(path) for path in paths):
                if time.monotonic() >= deadline:
                    raise RuntimeError("PTY creation timed out")
                time.sleep(0.05)
            for process in processes:
                if process.poll() is not None:
                    raise RuntimeError("softmodem startup failed")
            fds = [raw_open(path) for path in paths]
            server_fds = fds[:CHANNELS]
            caller_fds = fds[CHANNELS:]

            for index, fd in enumerate(caller_fds):
                write_all(fd, f"ATDT{100 + index}\r".encode())
            for fd in caller_fds:
                result = read_until(fd, MARKER, 45)
                if MARKER not in result:
                    raise RuntimeError("caller did not connect: " + repr(result))
            for fd in server_fds:
                result = read_until(fd, MARKER, 10)
                if MARKER not in result:
                    raise RuntimeError("server channel did not connect: " + repr(result))

            payloads = [bytes([0x40 + index]) * 64 for index in range(CHANNELS)]
            for fd, payload in zip(caller_fds, payloads):
                write_all(fd, payload)
            received = [read_count(fd, 64, 10) for fd in server_fds]
            if sorted(received) != sorted(payloads):
                raise RuntimeError("channel payload mismatch: " + repr(received))

            server_log = log_text(logs[0])
            for port in (11000, 11002, 11004, 11006):
                if f"RTP 127.0.0.1:{port}" not in server_log:
                    raise RuntimeError(f"RTP port {port} missing: {server_log}")
            print("multichannel SIP integration: 4 simultaneous CONNECT 33600, "
                  "RTP 11000/11002/11004/11006, four exact payloads")
        except Exception as error:
            details = [log_text(log) for log in logs]
            raise RuntimeError(f"{error}; logs={details}") from error
        finally:
            for fd in fds:
                os.close(fd)
            for process in processes:
                process.terminate()
            for process in processes:
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()
            for log in logs:
                log.close()


if __name__ == "__main__":
    main()
