# SIP Softmodem

A small clean-room SIP/RTP software modem for Linux, written in C11 by
[Gnomsa](mailto:gnomsa88@gmail.com). An incoming SIP call carrying G.711
A-law audio is answered by a software modem. Received serial bytes appear
on a Linux PTY; bytes written to that PTY are modulated back into RTP audio.

This repository contains an independent implementation. It does not contain
source code, binaries or VM images from other modem projects.

## Current scope

- SIP over one UDP socket with a configurable pool of simultaneous dialogs
- source-IP allowlist (optional)
- SDP offer/answer with PCMA only (static RTP payload type 8)
- RTP at 8 kHz with 20 ms packets
- sequence-aware 200 ms RTP jitter buffer without synthetic concealment audio
- G.711 A-law encoder and decoder
- selectable V.21 at 300 bit/s, V.22 at 1200 bit/s, V.22bis at 2400 bit/s,
  V.32/V.32bis sessions through 14400 bit/s, V.34 laboratory sessions from
  2400 through 33600 bit/s, and the older private experimental coherent QAM mode
- 2100 Hz answer tone followed by a short guard interval
- transparent PTY suitable for a terminal program or experimental PPP
- V.250-style command mode with `AT`, `ATDT`, `ATDP`, `ATDL`, `ATA`, `ATH`, `ATO`, `ATE`, `ATV`,
  `ATQ`, `ATI`, `ATS0`, `ATZ` and `AT+MS`
- incoming SIP calls produce `RING`; `ATA` and `ATS0` control the SIP answer
- incoming caller identity is reported as `+CLIP: "..."` alongside `RING`
- V.25 answer-tone detection and a V.22bis start-up state machine with 2400
  selection and 1200 fallback
- V.8 CM/JM/CJ framing, capability intersection and V.21(L/H) FSK transport
- V.8 ANSam generation and detection through G.711 A-law, including 15 Hz
  envelope modulation and 450 ms phase reversals
- standard V.32 GPC/GPA scramblers, differential coding, R/E rate words and
  exact 256 S + 16 S-bar + 1280 TRN symbol generation in the live SIP path
- standard V.32 1800 Hz/2400-baud A/B/C/D line transport and bidirectional
  4800-bit/s data path; deterministic PCMA tests currently report zero errors
- standard V.32 nonredundant 16-state 9600-bit/s mapping and bidirectional data
  path; its deterministic PCMA test transfers 1024 bytes each way with zero errors
- V.32bis rate negotiation, 8-state trellis coding, soft-decision Viterbi and
  normative 16/32/64/128-point mappings at 7200/9600/12000/14400 bit/s
- V.32 section 5.4 start-up controller covering caller/answer line states,
  64-symbol reversals, 16-symbol silence, two-identical-rate-word validation,
  automatic 9600/4800 intersection, E and 128-symbol final marking
- composite V.32 media session connected to SIP/RTP after V.8, with decoded E
  confirmation and 128 marking symbols before CONNECT; local two-process tests
  reach `CONNECT 4800` and `CONNECT 9600` and transfer exact PTY payloads
- RTP gaps are now reported by the jitter buffer after a short reordering wait;
  an active V.32 session requests in-band retraining with S, acknowledges with
  S-bar, repeats start-up without ending the SIP call, and suppresses data while
  retraining
- a confirmed RTP gap in the V.34 path invalidates the carrier immediately and
  reports `NO CARRIER` to the PTY instead of decoding a shifted QAM stream;
  V.34 in-band retraining remains to be implemented
- an 8191-byte session queue preserves PTY/PPP output written during retraining
  and releases it only after the new E and marking transition completes
- a closed slave PTY is probed with a 250 ms backoff, avoiding a `POLLHUP`
  busy-loop while no terminal program or `pppd` has the device open
- V.42 LAPM/HDLC is connected to the V.32/V.32bis media path, including XID,
  SABME/UA, modulo-128 ARQ, T401 retries and REJ recovery
- LAPM modulo-128 control fields encode and decode I, RR, RNR, REJ and SREJ
  frames with DLCI 0, C/R, N(S), N(R) and P/F fields
- an eight-frame LAPM ARQ window implements cumulative RR acknowledgement,
  RNR signalling and REJ retransmission; deterministic tests corrupt a middle
  frame, recover the exact ordered stream, and exercise sequence wrap 127 to 0
  as well as T401 recovery when the final I frame or its RR is lost
- the LAPM link controller establishes with SABME/UA, retries a lost SABME on
  T401, and performs orderly DISC/UA release (live data-path integration and
  live data-path integration are still pending)
- XID parameter fields negotiate directional N401 and window k values and
  intersect optional functions, including single-SREJ and 32-bit-FCS requests;
  the encoder includes the mandatory ISO/HDLC function mask and the complete
  XID information is tested inside an FCS-protected LAPM frame
- V.34 capability model for 2400–33600 bit/s, all six standardized symbol
  rates, directional/asymmetric rate selection, and exact INFO0/INFO1a/INFO1c
  bit and CRC framing
- V.34 Phase 2 probing-result selection with asymmetric symbol-rate limits,
  exact rational carrier and symbol clocks, and Phase 3 call/answer plans
- packet-streaming Phase 2 INFO transport uses 600-bit/s differential PSK on
  the 1200 Hz call carrier or the 2400 Hz answer carrier; the latter includes
  the 1800 Hz guard tone at the specified relative level. It carries every
  INFO0/INFO1 frame through PCMA while retaining fractional bit timing across
  arbitrary RTP-sized buffers
- streaming Phase 2 A and B tone generators/detectors preserve phase across
  packets, include the answer-side guard tone, and report 180-degree phase
  reversals independently of carrier presence
- the initial Phase 2 ranging controller waits 75 ms after V.8, exchanges and
  validates INFO0, performs the 50/40/10 ms A/B reversal sequence, and derives
  round-trip delay; its duplex test inserts 200 ms of packet buffering in each
  direction and measures the expected 400 ms round trip
- L1/L2 line-probing DSP generates and measures all 21 specified 150 Hz-grid
  tones from 150 through 3750 Hz with the four omitted carrier/guard bins, the
  prescribed initial phases, a 24-period/160 ms L1 window, and a 6 dB L1/L2
  level difference
- an autonomous spectral detector rejects A/B carrier and guard tones, finds
  an L1/L2 onset without packet-boundary hints, captures a full 160 ms window,
  and classifies the probe from its measured per-tone level
- the measured probe is converted into all six INFO1c projection entries;
  symbol-rate ceilings and configured rate masks are enforced, while spectral
  notches reduce the projection and high-frequency loss selects pre-emphasis
- a duplex probing controller sequences answer L1/L2, call L1/L2 and INFO1c,
  then selects the asymmetric mode and returns INFO1a; transmit and receive
  cursors remain independent across ten packets of simulated media latency
- one packet-streaming Phase 2 session now chains the initial 75 ms/INFO0/A/B
  ranging procedure, the final 10 ms answer tone, both line probes and the
  INFO1 exchange without an externally timed stage transition
- a full modem session consumes the Phase 2 result, configures independent
  transmit/receive symbol rates and carriers, then continues through Phase 3,
  Phase 4, B1 and queued asynchronous byte data using the same packet API
- normative Phase 3 S/S-bar, PP and four-point TRN symbol generation, including
  the separate call and answer scramblers, plus a PCMA training transmitter
- Phase 4 J-prime/TRN/MP/MP-prime/E plans and a PCMA MP Type 0 receiver
- V.34 B1 and data-mode shell mapping, differential encoding, 16/32/64-state
  trellis coding, QAM generation and inverse decoding at every symbol rate
- autonomous B1 carrier acquisition followed by a decision-directed data
  receiver; a 33600-bit/s laboratory test restores all 9408 superframe bits
  from fourteen 20 ms PCMA blocks despite injected phase and frequency offsets
- continuous V.34 data superframes retain carrier, symbol clock, scrambler,
  differential and trellis state without repeating B1
- an 8N1 asynchronous byte adapter accepts arbitrary PTY-sized writes, emits
  mark bits while idle, and reconstructs bytes from the primary-channel bits
- trellis-aware soft candidate search repairs hard QAM decisions before shell
  decoding; the PCMA test now carries arbitrary 8N1 byte patterns end to end
- a byte-oriented V.34 data channel automatically rotates superframes and
  exposes packet-sized PCMA generation/reception plus PTY-style reads/writes
- a duplex B1/data link runs caller and answerer B1 concurrently, queues bytes
  written during training, and switches both RTP directions to data packets
  after the two B1 receivers confirm their state
- autonomous Phase 4 PCMA streams and receivers validate J-prime, 512 TRN
  symbols, MP, acknowledged MP-prime and E in both directions
- the E-to-B1 transition carries fractional symbol-clock and carrier phase
  state into asymmetric 33600/28800 data channels without reacquisition
- a packet-oriented V.34 session controller now drives Phase 3, Phase 4, B1
  and continuous data through one generate/receive/read/write API; bytes
  written during training remain queued until the modem reports connected
- transmit and receive training stages advance independently across buffered
  media; an asymmetric 7/13-packet latency test uses the live receive-before-
  generate scheduler order and covers J-to-J-prime, E-to-B1 and B1-to-data
  transitions without dropping the first packet of the following stage

This is an early laboratory modem. The initial demodulator assumes a clean,
low-jitter signal and does not yet implement full timing recovery, adaptive
equalisation, echo cancellation, all SIP transaction timers, RTCP or TCP SIP.
V.34 is advertised through V.8 and connected to the live SIP/RTP/PTY path, but
has only been validated between two copies of this program over loss-free PCMA.
The 4800/9600 implementation remains a clean-room laboratory modem. Standard
V.32bis start-up and `CONNECT 9600` have been observed against a physical CSD
endpoint through SIP/PCMA, but error-free user data has not yet been demonstrated.
Timing acquisition and adaptive equalisation are present; carrier recovery,
long-term clock tracking, echo cancellation and retraining still need validation
on physical telephone paths.

## Build and test

Debian needs only the normal C toolchain:

```sh
sudo apt install build-essential
make
make test
make link-test
make integration-test
make integration-v34-test
# Longer live matrix covering every V.34 rate:
make integration-v34-all-test
# Timed 4096-byte V.34 payload:
make integration-v34-throughput-test
# Four simultaneous calls on one SIP port:
make integration-multichannel-test
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
does. With four channels, the default base path produces `ttyMODEM0` through
`ttyMODEM3`; each PTY must have only one serial-side owner. A minimal
experimental PPP server command after carrier establishment is:

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
| `SOFTMODEM_RTP_PORT` | `10000` | RTP base port; channel N uses base + 2N |
| `SOFTMODEM_CHANNELS` | `4` | simultaneous modem channels on the shared SIP socket; range 1–32 |
| `SOFTMODEM_PROTOCOLS` | `ALL` | allowed modes: `V21,V22,V22BIS,V32,V32BIS,V34`; comma-separated |
| `SOFTMODEM_MAX_RATE` | `33600` | maximum permitted rate; highest enabled mode is selected |
| `SOFTMODEM_V8` | `1` | enable ANSam and V.8 CM/JM/CJ family negotiation; `0` uses legacy start-up |
| `SOFTMODEM_ALLOWED_IPS` | empty | comma-separated SIP source addresses; empty allows all |
| `SOFTMODEM_OUTBOUND_HOST` | empty | SIP proxy/SBC address used for outgoing `ATD` calls |
| `SOFTMODEM_OUTBOUND_PORT` | `5060` | SIP proxy/SBC UDP port |
| `SOFTMODEM_CALLER_ID` | `modem` | SIP user placed in the `From` URI for outgoing calls |
| `SOFTMODEM_TTY` | `/tmp/ttySOFTMODEM0` | PTY base path; trailing digits are replaced by the channel index when multichannel |
| `SOFTMODEM_USER_AGENT` | `SIP-Softmodem/0.1` | value used in the SIP `Server` header |
| `SOFTMODEM_SDP_ORIGIN` | `softmodem` | username in the SDP `o=` line |
| `SOFTMODEM_SDP_NAME` | `SIP Softmodem` | text in the SDP `s=` line |

Identity settings reject CR and LF characters. The source allowlist is a simple
exact IPv4 match, not a replacement for a firewall on an untrusted network.

`ALL` chooses the highest mode allowed by `SOFTMODEM_MAX_RATE`. V.34 uses the
highest permitted 2400-bit/s step through 33600; selecting `V32BIS` explicitly
retains its automatic choice through 14400, 12000, 9600 and 7200 bit/s.
`EXPERIMENTAL_QAM` explicitly enables the private 4800/9600
loopback waveform; it is deliberately not named V.32 and is never selected by
`ALL`. V.22bis now performs its in-band 2400/1200 selection. The V.8 codec,
FSK transport, ANSam detector and automode state machine are enabled by default.
Simple ANS bypasses V.8, and an answering modem falls back after the ANSam
timeout when a legacy caller does not send CM.

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

`make integration-v32-test` performs the same full-process test with V.8 and
the composite V.32 path, negotiates a V.42 LAPM session (XID and SABME/UA),
checks `CONNECT 9600`, and verifies the PTY payload through HDLC/ARQ.
`make integration-v32-4800-test` exercises the corresponding 4800-bit/s path.
`make integration-v32bis-test` runs two complete processes, checks
`CONNECT 14400`, establishes V.42 and verifies an exact PTY payload.
`make integration-v34-test` runs the complete V.8, Phase 2, Phase 3, Phase 4,
B1 and continuous-data path, checks `CONNECT 33600`, and verifies the exact PTY
payload through two local SIP/RTP processes.
`make integration-v34-all-test` repeats that test at every 2400-bit/s step from
2400 through 33600. The loss-free local matrix currently reaches the requested
`CONNECT` rate and transfers the exact payload at all fourteen rates.
`make integration-v34-throughput-test` transfers 4096 deterministic binary
bytes after `CONNECT 33600`, checks every byte and reports measured PTY payload
throughput independently of the negotiated raw line rate. A representative
loss-free localhost run measured about 2505 payload bytes/s (20.0 kbit/s), or
74.5% of the 3360 bytes/s 8N1 ceiling; this includes superframe fill and drain
latency over the finite 4096-byte measurement.
`make integration-multichannel-test` starts one four-channel endpoint on a
single SIP port, places four calls concurrently, verifies four `CONNECT 33600`
results, checks RTP ports 11000/11002/11004/11006 and compares an independent
payload received through every server-side PTY.
The deterministic V.42-over-V.32bis test currently transfers 1000 exact bytes
at about 400, 481, 562 and 610 application bytes/s on 7200, 9600, 12000 and
14400-bit/s lines. This includes LAPM and continuous HDLC idle traffic.

The V.42 implementation includes 16-bit FCS, bit stuffing, modulo-128 I and S
frames, cumulative acknowledgements, REJ retransmission, T401 retries, XID
parameter negotiation and SABME/UA establishment.  It is exercised between two
copies of this program; interoperability with a hardware LAPM modem has not yet
been demonstrated.

## Physical CSD interoperability status

On 2026-08-29 the calling side was tested through a direct SIP trunk carrying
20 ms PCMA packets to a physical cellular CSD endpoint. The remote address,
telephone number and caller identity are intentionally omitted from this public
document.

The following has been observed on successful calls:

- SIP `INVITE`, provisional responses, `200 OK`, `ACK`, bidirectional PCMA RTP
  and remote `BYE` complete normally;
- V.8 selects the V.32 family and V.32bis selects 9600 bit/s;
- the physical endpoint accepts the corrected whole-word R2-to-E transition;
- after preserving the rate-signal scrambler and differential state, the remote
  E response was observed about 0.4 seconds after local E instead of the earlier
  invalid 18-second detection;
- the PTY reports `CONNECT 9600`;
- ten-phase B1 acquisition reduced measured training EVM from 90.26% to 41.88%
  on one comparable successful run and selected phase 1/10;
- the decoded stream began to contain repeated PPP-like flag and escape octets.

These results prove physical start-up interoperability, not a working data
link. Offline inspection of that capture found zero PPP frames with a valid
16-bit FCS. Therefore PPP, transparent byte transfer, effective throughput and
answer-side interoperability with a physical modem remain unconfirmed.

Subsequent code corrects a four-symbol receiver-delay discontinuity at the
E-to-B1 handoff and estimates carrier offset only when the B1 correlation is
coherent and within the V.32bis ±7 Hz receiver requirement. Those two changes
passed the complete deterministic test suite, but have not yet been validated
on a clean physical call: later attempts either received no remote E or suffered
large RTP gaps while the test host was busy with backup work. For meaningful
physical measurements, avoid simultaneous disk/network-heavy jobs and reject
any run containing RTP gaps.

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
