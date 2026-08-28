# SIP Softmodem

A small clean-room SIP/RTP software modem for Linux, written in C11 by
[Gnomsa](mailto:gnomsa88@gmail.com). An incoming SIP call carrying G.711
A-law audio is answered by a software modem. Received serial bytes appear
on a Linux PTY; bytes written to that PTY are modulated back into RTP audio.

This repository contains an independent implementation. It does not contain
source code, binaries or VM images from other modem projects.

## Current scope

- SIP over UDP, one incoming dialog
- source-IP allowlist (optional)
- SDP offer/answer with PCMA only (static RTP payload type 8)
- RTP at 8 kHz with 20 ms packets
- sequence-aware 200 ms RTP jitter buffer without synthetic concealment audio
- G.711 A-law encoder and decoder
- selectable V.21 at 300 bit/s, V.22 at 1200 bit/s, V.22bis at 2400 bit/s,
  and experimental coherent QAM at 4800 or 9600 bit/s
- 2100 Hz answer tone followed by a short guard interval
- transparent PTY suitable for a terminal program or experimental PPP
- V.250-style command mode with `AT`, `ATDT`, `ATDP`, `ATDL`, `ATA`, `ATH`, `ATO`, `ATE`, `ATV`,
  `ATQ`, `ATI`, `ATS0`, `ATZ` and `AT+MS`
- incoming SIP calls produce `RING`; `ATA` and `ATS0` control the SIP answer
- incoming caller identity is reported as `+CLIP: "..."` alongside `RING`
- V.25 answer-tone detection and a V.22bis start-up state machine with 2400
  selection and 1200 fallback
- V.8 CM/JM/CJ framing, capability intersection and V.21(L/H) FSK transport;
  these components are tested but not yet enabled in the live call path
- V.8 ANSam generation and detection through G.711 A-law, including 15 Hz
  envelope modulation and 450 ms phase reversals

This is an early laboratory modem. The initial demodulator assumes a clean,
low-jitter signal and does not yet implement full carrier/timing recovery,
adaptive equalisation, all SIP transaction timers, RTCP, TCP SIP or V.34.
The 4800/9600 implementation is a clean-room laboratory waveform between two
instances of this program. It is not yet an interoperable ITU-T V.32 modem:
trellis coding, echo cancellation and adaptive equalisation remain to be
implemented. V.8 still needs ANSam and end-to-end fallback validation.

## Build and test

Debian needs only the normal C toolchain:

```sh
sudo apt install build-essential
make
make test
make link-test
make integration-test
```

Run in the foreground:

```sh
sudo install -d /run/sip-softmodem
sudo env \
  SOFTMODEM_PUBLIC_IP=192.0.2.20 \
  SOFTMODEM_ALLOWED_IPS=192.0.2.10 \
  SOFTMODEM_TTY=/run/sip-softmodem/ttyMODEM0 \
  ./sip-softmodem
```

Configure the SIP peer for UDP port 5060, PCMA only, a 20 ms packet time and
RTP port 10000. Disable transcoding, VAD, echo cancellation and packet-loss
concealment. Replace the example addresses with the actual modem host and SIP
peer addresses.

Open the serial side after startup:

```sh
picocom --baud 115200 /run/sip-softmodem/ttyMODEM0
```

The PTY baud setting does not determine the line speed; the selected modem mode
does.
Only one process should own the PTY. A minimal experimental PPP server command
after carrier establishment is:

```sh
sudo pppd /run/sip-softmodem/ttyMODEM0 115200 local passive noauth \
  10.77.0.1:10.77.0.2 nodetach debug
```

## Configuration

All settings are environment variables. See
[`config/softmodem.env.example`](config/softmodem.env.example).

| Variable | Default | Meaning |
|---|---|---|
| `SOFTMODEM_BIND_IP` | `0.0.0.0` | local SIP and RTP bind address |
| `SOFTMODEM_PUBLIC_IP` | `127.0.0.1` | address advertised in Contact and SDP |
| `SOFTMODEM_SIP_PORT` | `5060` | SIP UDP port |
| `SOFTMODEM_RTP_PORT` | `10000` | RTP UDP port |
| `SOFTMODEM_PROTOCOLS` | `ALL` | allowed standard modes: `V21,V22,V22BIS`; comma-separated |
| `SOFTMODEM_MAX_RATE` | `2400` | maximum permitted rate; highest enabled mode is selected |
| `SOFTMODEM_ALLOWED_IPS` | empty | comma-separated SIP source addresses; empty allows all |
| `SOFTMODEM_OUTBOUND_HOST` | empty | SIP proxy/SBC address used for outgoing `ATD` calls |
| `SOFTMODEM_OUTBOUND_PORT` | `5060` | SIP proxy/SBC UDP port |
| `SOFTMODEM_TTY` | `/tmp/ttySOFTMODEM0` | stable symlink to the allocated PTY |
| `SOFTMODEM_USER_AGENT` | `SIP-Softmodem/0.1` | value used in the SIP `Server` header |
| `SOFTMODEM_SDP_ORIGIN` | `softmodem` | username in the SDP `o=` line |
| `SOFTMODEM_SDP_NAME` | `SIP Softmodem` | text in the SDP `s=` line |

Identity settings reject CR and LF characters. The source allowlist is a simple
exact IPv4 match, not a replacement for a firewall on an untrusted network.

`ALL` currently chooses 2400. Setting the maximum to 1200 or 300 selects a
lower mode. `EXPERIMENTAL_QAM` explicitly enables the private 4800/9600
loopback waveform; it is deliberately not named V.32 and is never selected by
`ALL`. V.22bis now performs its in-band 2400/1200 selection. The V.8 codec,
FSK transport, ANSam detector and logical automode state machine exist, but
family selection remains disabled until the combined end-to-end fallback test
is complete.

## Measured loopback status

`make link-test` connects two independent DSP instances through the project's
G.711 A-law codec in both directions. In the deterministic, loss-free test it
currently reports zero bit errors at every implemented rate. The measured
payload throughput at 9600 is about 948 bytes/s and at 4800 about 474 bytes/s;
UART framing accounts for most of the difference from the raw bit rate. This
test establishes compatibility between two copies of this software only, not
compatibility with a telephone-network or hardware modem.

`make integration-test` starts two complete processes on local SIP and RTP
ports, dials with `ATDT`, negotiates V.22bis, checks `CONNECT 2400` and verifies
an exact payload through both PTYs. It needs permission to open local UDP
sockets.

## Install as a service

```sh
sudo install -m 0755 sip-softmodem /usr/local/sbin/sip-softmodem
sudo install -m 0644 config/softmodem.env.example /etc/sip-softmodem.conf
sudo install -m 0644 systemd/sip-softmodem.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now sip-softmodem
```

Edit `/etc/sip-softmodem.conf` before starting the service.

## License

MIT. Copyright © 2026 Gnomsa `<gnomsa88@gmail.com>`.
