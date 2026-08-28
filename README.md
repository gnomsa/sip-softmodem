# SIP Softmodem

A small clean-room SIP/RTP software modem for Linux, written in C11 by
[Gnomsa](mailto:gnomsa88@gmail.com). An incoming SIP call carrying G.711
A-law audio is answered by a software V.21 modem. Received serial bytes appear
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
- V.21 answer channel: TX 1650/1850 Hz, RX 980/1180 Hz, 300 bit/s, 8N1
- 2100 Hz answer tone followed by a short guard interval
- transparent PTY suitable for a terminal program or experimental PPP

This is an early laboratory modem. The initial demodulator assumes a clean,
low-jitter signal and does not yet implement full carrier/timing recovery,
adaptive equalisation, all SIP transaction timers, RTCP, TCP SIP, V.22 or
V.34. Those are planned work, not current claims.

## Build and test

Debian needs only the normal C toolchain:

```sh
sudo apt install build-essential
make
make test
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

The PTY baud setting does not determine the line speed: V.21 remains 300 bit/s.
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
| `SOFTMODEM_ALLOWED_IPS` | empty | comma-separated SIP source addresses; empty allows all |
| `SOFTMODEM_TTY` | `/tmp/ttySOFTMODEM0` | stable symlink to the allocated PTY |
| `SOFTMODEM_USER_AGENT` | `SIP-Softmodem/0.1` | value used in the SIP `Server` header |
| `SOFTMODEM_SDP_ORIGIN` | `softmodem` | username in the SDP `o=` line |
| `SOFTMODEM_SDP_NAME` | `SIP Softmodem` | text in the SDP `s=` line |

Identity settings reject CR and LF characters. The source allowlist is a simple
exact IPv4 match, not a replacement for a firewall on an untrusted network.

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
