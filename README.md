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
- idle channel polls wake at least once per second, so `SIGTERM` shuts down all
  channel threads cleanly instead of making systemd wait for `TimeoutStopSec`
  and kill the process; a four-channel process regression test covers this
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
endpoint through SIP/PCMA. Error-free incoming PPP/LCP frames have also been
decoded from a physical 9600-bit/s call. Timing acquisition and adaptive
equalisation are present; carrier recovery, long-term clock tracking, echo
cancellation and successful retrain completion still need validation on
physical telephone paths. A failed B1 acquisition is quality-gated and starts
in-band retraining instead of exposing a false carrier to the PTY.

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

On 2026-08-29 and 2026-08-30 the calling side was tested through a direct SIP
trunk carrying
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
- fractional matched-filter interpolation makes all ten B1 timing phases real
  sampling phases rather than nearest-sample aliases;
- B1 acquisition jointly searches a sequence-boundary offset of minus eight
  through plus eight symbols as well as timing and differential state. On a
  saved physical run it selected minus one symbol, reduced replay EVM from
  31.85% to 27.44%, and retained all ten valid PPP frames;
- joint timing and four-state differential acquisition reduced B1 EVM from
  94.80% to 20.90% on a clean successful run;
- B1 diagnostics report observed-to-expected RMS input level and the number of
  known symbols used. A failed 106.64% EVM capture had 1.804x input level,
  while two successful captures measured 1.718x and 1.793x, proving that its
  post-E carrier was present rather than silent;
- carrier drift is estimated after equalisation and reported as a diagnostic,
  but is not applied as an abrupt phase step after B1. Offline application of
  even small 2.27 Hz and 1.27 Hz estimates corrupted every PPP FCS, whereas
  leaving the trained decision-directed equaliser continuous preserved 7/7
  and 10/11 valid frames respectively. A continuous PLL remains future work;
- the E detector can reconstruct the exact standard E word, scrambler state
  and differential state when one of its eight carrier symbols is received
  incorrectly. On a later gap-free capture this moved E detection about 21
  seconds earlier, reduced the following B1 EVM from 107.68% to 19.36%, and
  recovered seven consecutive PPP frames with valid FCS;
- one-symbol E reconstruction is limited to the first 256 candidate words
  after R3. Exact E remains detectable later, but an approximate match is not
  allowed to occur indefinitely in arbitrary data; a rejected physical run
  had previously produced such a false match only after 718 words;
- a live call after installing that recovery reached `CONNECT 9600` with an
  uncorrected exact E word and 31.85% B1 EVM. Its 2076 inbound RTP packets had
  no sequence gaps, and all ten received 35-byte PPP/LCP frames had valid FCS;
- a subsequent live `pppd` call proved the user-data path in both directions:
  the physical peer acknowledged the local LCP Configure-Request, the local
  peer decoded its LCP requests and returned Configure-Reject/Configure-Nak,
  and the physical peer received those replies and changed its next request.
  IPCP was not reached because that endpoint requires CHAP-MD5 credentials;
- a later live call reached `CONNECT 9600` with 31.02% B1 EVM and one corrected
  E carrier symbol. When the remote side ended the call, the PTY was replaced
  and `pppd` immediately reported `Modem hangup`; it no longer consumed echoed
  post-call LCP bytes or diagnosed a false serial-line loopback;
- V.14 deleted-stop-bit handling recovered the character immediately before a
  shared PPP flag. Offline replay of the captured PCMA then decoded ten
  consecutive 35-byte PPP LCP Configure-Request frames, all ten with a valid
  CRC-16/FCS;
- that capture contained 2155 inbound RTP packets with no sequence gaps. The
  only non-160 timestamp increments were three startup increments before the
  continuous media stream settled.

These results prove physical start-up and error-free incoming user-data
interoperability on a successful call and bidirectional LCP transport. A
complete authenticated PPP/IP session, effective application throughput and
answer-side interoperability with a physical modem remain unconfirmed.

Physical acquisition is not yet reliable on every attempt. Another gap-free
call had 111.94% B1 EVM and produced no valid data; none of the timing or
differential candidates correlated with the known B1 sequence. B1 acquisition
now requires no more than 60% EVM. A worse result is rejected before `CONNECT`
or user data is reported. During data and the bounded acquisition waits, the
receiver watches for the role-specific retrain trigger required by V.32bis:
600/3000 Hz for a call-mode modem and 1800 Hz for an answer-mode modem, present
for more than 128 symbol intervals. If no remote trigger is detected during
the 384-symbol B1 arbitration window, the transmitter initiates retraining.
The same detector runs while waiting for remote E. If neither E nor a retrain
trigger arrives
within 512 candidate words (about 1.7 seconds), the modem requests retraining
instead of remaining in E until the SIP call times out. Retrain state, A/B and
C/D counters, E-word count and the B1 arbitration timer are included in the
service log. Waiting for remote R3 is bounded as well: the modem recognises an
R3 or initiates its own retrain after 7200 rate symbols (3 seconds) without
one. The expected second receiver-conditioning signal is handled exclusively
by the training/rate scanners: its periodic S/Sbar/TRN content is not passed
to the generic in-data retrain-tone detector. This avoids both the previously
observed 27-second R2/R3 stall and a false retrain at about 0.60 seconds. A
standard retrain now returns to the role-specific
start-up point: repeated carrier state A (AA) in call mode, or alternating A/C
in answer mode. It then repeats synchronization, receiver conditioning and
rate negotiation. An earlier shortened A/B-to-C/D implementation was accepted
by the physical peer and reached a second `S/Sbar/TRN`, but the peer did not
subsequently send R3; that live result motivated replacing the shortcut with
the complete procedure. A physical call with the complete implementation then
appeared to perform four consecutive peer-initiated retrains, each traversing
`AA`, `CC`, first receiver conditioning, R1, second receiver conditioning and
R2/R3. Offline timing showed that each restart occurred exactly as the peer's
expected second receiver conditioning ended; the general tone detector had
misclassified that training rather than receiving a new retrain request.

The TRN-to-R2 transmitter handoff now also preserves the eight-symbol RRC
history and continues the TRN scrambler instead of resetting it. In the first
live call after fixing the R3 arbitration, the physical modem proceeded to E,
accepted B1 at 30.77% EVM, reported `CONNECT 9600` and delivered user bytes for
about nine seconds. It then requested a genuine in-data retrain; that retrain
completed through the next R2/R3, where the peer did not return R3 before the
call ended. This follows the retrain procedure and
receiver-conditioning segments in
[ITU-T V.32 section 5](https://www.itu.int/rec/T-REC-V.32/en). This remains an
equaliser/carrier-training problem
rather than a SIP or packet-loss problem. For meaningful measurements, avoid
simultaneous disk/network-heavy jobs, reject runs containing RTP gaps, and
record B1 EVM alongside data-layer FCS results.

### Offline PCMA replay

Two optional diagnostic tools make a physical call repeatable without placing
another call. They are not built by the default target:

```sh
make tools/pcap_pcma_extract tools/v32_pcma_replay tools/ppp_fcs_check
editcap -F pcap capture.pcapng capture.pcap
tools/pcap_pcma_extract capture.pcap 10000 inbound.pcma
tools/v32_pcma_replay inbound.pcma decoded.bin 330 9600
tools/ppp_fcs_check decoded.bin
```

Append `--trace` after the rate to print every reconstructed start-up/retrain
phase transition with its input block number and relative time.

`pcap_pcma_extract` accepts classic pcap with Ethernet, raw IPv4, Linux cooked
v1 or Linux cooked v2 encapsulation. It selects RTP/PCMA payload type 8 by UDP
destination port and reports packet count, sequence gaps and non-160 timestamp
steps. `v32_pcma_replay` feeds the raw payload through the same calling-side
standard V.32bis receiver used by the service and reports start-up state, rate,
B1 timing/differential/alignment selection, acquisition status, retrain state,
raw carrier correlation, EVM and decoded byte count. Capture files and
extracted payloads can contain private
telephone and network metadata and should remain outside the repository.
`ppp_fcs_check` removes RFC 1662 escaping and counts frames whose received
CRC-16 has the standard `0xf0b8` good residue.

After an established carrier is lost, the channel writes `NO CARRIER`, closes
the old PTY master and atomically replaces the public symlink with a fresh PTY.
This gives programs such as `pppd` an immediate hangup/EOF instead of allowing
their post-disconnect PPP bytes to enter AT command mode and be echoed back as
a false loopback. A terminal program should reopen the same public symlink for
the next call. If SIP disconnects before `CONNECT`, the channel instead reports
`NO CARRIER` without replacing the PTY, so an `ATD` terminal or `chat` script
can fail immediately while retaining the same open descriptor. Repeated RTP
gaps still request retraining, but the diagnostic is logged only once per call.

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
