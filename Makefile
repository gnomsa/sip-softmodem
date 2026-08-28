CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDLIBS += -lm -lutil

SRC := src/main.c src/at.c src/pcma.c src/rtp.c src/jitter.c src/sip.c src/v21.c src/v22.c src/v22bis.c src/v22_handshake.c src/v32.c src/v32_std.c src/v32bis_trellis.c src/v32bis_map.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_e.c src/v32_data.c src/v32_qam.c src/v32_startup.c src/v32_retrain.c src/v32_session.c src/v42_hdlc.c src/v42_lapm.c src/v42_arq.c src/v42_link.c src/v42_xid.c src/v42_session.c src/v42_stream.c src/v42_v32.c src/v8.c src/v8_fsk.c src/v8_session.c src/ansam.c src/tone_detector.c src/pty.c
SRC += src/v32bis_viterbi.c
SRC += src/v32bis_data.c
SRC += src/v32bis_qam.c
SRC += src/v34_caps.c
SRC += src/v34_info.c
SRC += src/v34_phase2.c
SRC += src/v34_timing.c
SRC += src/v34_phase3.c
SRC += src/v34_training_symbols.c
SRC += src/v34_training_tx.c
SRC += src/v34_phase3_stream.c
SRC += src/v34_j_detector.c
SRC += src/v34_training_rx.c
SRC += src/v34_mp.c
SRC += src/v34_phase4.c
SRC += src/v34_mp_receiver.c
SRC += src/v34_framing.c
SRC += src/v34_b1.c
SRC += src/v34_mapper.c
SRC += src/v34_data_mapper.c
SRC += src/v34_trellis.c
SRC += src/v34_qam_tx.c
SRC += src/v34_b1_stream.c
SRC += src/v34_data_stream.c
SRC += src/v34_data_decoder.c
SRC += src/v34_b1_receiver.c
SRC += src/v34_data_receiver.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean test link-test integration-test integration-v32-test integration-v32-4800-test integration-v32bis-test
all: sip-softmodem

sip-softmodem: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

test: tests/test_core tests/test_tones tests/test_v22_handshake tests/test_v8 tests/test_v8_fsk tests/test_v8_session tests/test_ansam tests/test_v32_std tests/test_v32bis_trellis tests/test_v32bis_map tests/test_v32bis_viterbi tests/test_v32bis_data tests/test_v32bis_qam tests/test_v32bis_link tests/test_v32_training tests/test_v32_line tests/test_v32_rate tests/test_v32_e tests/test_v32_data tests/test_v32_qam tests/test_v32_startup tests/test_v32_startup_link tests/test_v32_retrain tests/test_v32_session tests/test_v42_hdlc tests/test_v42_lapm tests/test_v42_arq tests/test_v42_link tests/test_v42_xid tests/test_v42_session tests/test_v42_stream tests/test_v42_v32 tests/test_v34_caps tests/test_v34_info tests/test_v34_info1 tests/test_v34_phase2 tests/test_v34_timing tests/test_v34_phase3 tests/test_v34_training_symbols tests/test_v34_training_tx tests/test_v34_phase3_stream tests/test_v34_j_detector tests/test_v34_training_rx tests/test_v34_phase3_link tests/test_v34_mp tests/test_v34_phase4 tests/test_v34_phase4_link tests/test_v34_mp_receiver tests/test_v34_framing tests/test_v34_b1 tests/test_v34_mapper tests/test_v34_data_mapper tests/test_v34_trellis tests/test_v34_b1_stream tests/test_v34_data_stream tests/test_v34_data_decoder tests/test_v34_b1_receiver
	./tests/test_core
	./tests/test_tones
	./tests/test_v22_handshake
	./tests/test_v8
	./tests/test_v8_fsk
	./tests/test_v8_session
	./tests/test_ansam
	./tests/test_v32_std
	./tests/test_v32bis_trellis
	./tests/test_v32bis_map
	./tests/test_v32bis_viterbi
	./tests/test_v32bis_data
	./tests/test_v32bis_qam
	./tests/test_v32bis_link
	./tests/test_v32_training
	./tests/test_v32_line
	./tests/test_v32_rate
	./tests/test_v32_e
	./tests/test_v32_data
	./tests/test_v32_qam
	./tests/test_v32_startup
	./tests/test_v32_startup_link
	./tests/test_v32_retrain
	./tests/test_v32_session
	./tests/test_v42_hdlc
	./tests/test_v42_lapm
	./tests/test_v42_arq
	./tests/test_v42_link
	./tests/test_v42_xid
	./tests/test_v42_session
	./tests/test_v42_stream
	./tests/test_v42_v32
	./tests/test_v34_caps
	./tests/test_v34_info
	./tests/test_v34_info1
	./tests/test_v34_phase2
	./tests/test_v34_timing
	./tests/test_v34_phase3
	./tests/test_v34_training_symbols
	./tests/test_v34_training_tx
	./tests/test_v34_phase3_stream
	./tests/test_v34_j_detector
	./tests/test_v34_training_rx
	./tests/test_v34_phase3_link
	./tests/test_v34_mp
	./tests/test_v34_phase4
	./tests/test_v34_phase4_link
	./tests/test_v34_mp_receiver
	./tests/test_v34_framing
	./tests/test_v34_b1
	./tests/test_v34_mapper
	./tests/test_v34_data_mapper
	./tests/test_v34_trellis
	./tests/test_v34_b1_stream
	./tests/test_v34_data_stream
	./tests/test_v34_data_decoder
	./tests/test_v34_b1_receiver

link-test: tests/dsp_link
	./tests/dsp_link

integration-test: sip-softmodem
	python3 tests/integration_local.py

integration-v32-test: sip-softmodem
	SOFTMODEM_TEST_PROTOCOL=V32 SOFTMODEM_TEST_RATE=9600 python3 tests/integration_local.py

integration-v32-4800-test: sip-softmodem
	SOFTMODEM_TEST_PROTOCOL=V32 SOFTMODEM_TEST_RATE=4800 python3 tests/integration_local.py

integration-v32bis-test: sip-softmodem
	SOFTMODEM_TEST_PROTOCOL=V32BIS SOFTMODEM_TEST_RATE=14400 python3 tests/integration_local.py

tests/dsp_link: tests/dsp_link.c src/v21.c src/v22.c src/v22bis.c src/v22_handshake.c src/v32.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_core: tests/test_core.c src/at.c src/pcma.c src/rtp.c src/jitter.c src/sip.c src/v21.c src/v22.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_tones: tests/test_tones.c src/tone_detector.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v22_handshake: tests/test_v22_handshake.c src/v22_handshake.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v8: tests/test_v8.c src/v8.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v8_fsk: tests/test_v8_fsk.c src/v8.c src/v8_fsk.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v8_session: tests/test_v8_session.c src/v8.c src/v8_session.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_ansam: tests/test_ansam.c src/ansam.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_std: tests/test_v32_std.c src/v32_std.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_trellis: tests/test_v32bis_trellis.c src/v32bis_trellis.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_map: tests/test_v32bis_map.c src/v32bis_map.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_viterbi: tests/test_v32bis_viterbi.c src/v32bis_viterbi.c src/v32bis_trellis.c src/v32bis_map.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_data: tests/test_v32bis_data.c src/v32bis_data.c src/v32bis_viterbi.c src/v32bis_trellis.c src/v32bis_map.c src/v32_std.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_qam: tests/test_v32bis_qam.c src/v32bis_qam.c src/v32bis_map.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32bis_link: tests/test_v32bis_link.c src/v32bis_data.c src/v32bis_qam.c src/v32bis_viterbi.c src/v32bis_trellis.c src/v32bis_map.c src/v32_std.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_training: tests/test_v32_training.c src/v32_std.c src/v32_training.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_line: tests/test_v32_line.c src/v32_std.c src/v32_training.c src/v32_line.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_rate: tests/test_v32_rate.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_rate.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_e: tests/test_v32_e.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_e.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_data: tests/test_v32_data.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_data.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_qam: tests/test_v32_qam.c src/v32_std.c src/v32_training.c src/v32_data.c src/v32_qam.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_startup: tests/test_v32_startup.c src/v32_std.c src/v32_startup.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_startup_link: tests/test_v32_startup_link.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_startup.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_retrain: tests/test_v32_retrain.c src/v32_retrain.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_session: tests/test_v32_session.c src/v32_std.c src/v32bis_trellis.c src/v32bis_map.c src/v32bis_viterbi.c src/v32bis_data.c src/v32bis_qam.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_e.c src/v32_startup.c src/v32_retrain.c src/v32_data.c src/v32_qam.c src/v32_session.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_hdlc: tests/test_v42_hdlc.c src/v42_hdlc.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_lapm: tests/test_v42_lapm.c src/v42_hdlc.c src/v42_lapm.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_arq: tests/test_v42_arq.c src/v42_hdlc.c src/v42_lapm.c src/v42_arq.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_link: tests/test_v42_link.c src/v42_hdlc.c src/v42_lapm.c src/v42_link.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_xid: tests/test_v42_xid.c src/v42_hdlc.c src/v42_lapm.c src/v42_xid.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_session: tests/test_v42_session.c src/v42_hdlc.c src/v42_lapm.c src/v42_arq.c src/v42_link.c src/v42_xid.c src/v42_session.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_stream: tests/test_v42_stream.c src/v42_hdlc.c src/v42_stream.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v42_v32: tests/test_v42_v32.c src/v32_std.c src/v32bis_trellis.c src/v32bis_map.c src/v32bis_viterbi.c src/v32bis_data.c src/v32bis_qam.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_e.c src/v32_startup.c src/v32_retrain.c src/v32_data.c src/v32_qam.c src/v32_session.c src/v42_hdlc.c src/v42_lapm.c src/v42_arq.c src/v42_link.c src/v42_xid.c src/v42_session.c src/v42_stream.c src/v42_v32.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_caps: tests/test_v34_caps.c src/v34_caps.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_info: tests/test_v34_info.c src/v34_info.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_info1: tests/test_v34_info1.c src/v34_info.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase2: tests/test_v34_phase2.c src/v34_phase2.c src/v34_caps.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_timing: tests/test_v34_timing.c src/v34_timing.c src/v34_caps.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase3: tests/test_v34_phase3.c src/v34_phase3.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_training_symbols: tests/test_v34_training_symbols.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_training_tx: tests/test_v34_training_tx.c src/v34_training_tx.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase3_stream: tests/test_v34_phase3_stream.c src/v34_phase3_stream.c src/v34_phase3.c src/v34_timing.c src/v34_training_symbols.c src/v34_training_tx.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_j_detector: tests/test_v34_j_detector.c src/v34_j_detector.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_training_rx: tests/test_v34_training_rx.c src/v34_training_rx.c src/v34_training_tx.c src/v34_timing.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase3_link: tests/test_v34_phase3_link.c src/v34_phase3_stream.c src/v34_phase3.c src/v34_timing.c src/v34_training_symbols.c src/v34_training_tx.c src/v34_training_rx.c src/v34_j_detector.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_mp: tests/test_v34_mp.c src/v34_mp.c src/v34_info.c src/v34_caps.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase4: tests/test_v34_phase4.c src/v34_phase4.c src/v34_mp.c src/v34_info.c src/v34_caps.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_phase4_link: tests/test_v34_phase4_link.c src/v34_phase4.c src/v34_mp.c src/v34_mp_receiver.c src/v34_info.c src/v34_training_symbols.c src/v34_training_tx.c src/v34_training_rx.c src/v34_timing.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_mp_receiver: tests/test_v34_mp_receiver.c src/v34_mp_receiver.c src/v34_mp.c src/v34_info.c src/v34_caps.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_framing: tests/test_v34_framing.c src/v34_framing.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_b1: tests/test_v34_b1.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_mapper: tests/test_v34_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_data_mapper: tests/test_v34_data_mapper.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_trellis: tests/test_v34_trellis.c src/v34_trellis.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_b1_stream: tests/test_v34_b1_stream.c src/v34_b1_stream.c src/v34_qam_tx.c src/v34_trellis.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c src/v34_training_rx.c src/v34_timing.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_data_stream: tests/test_v34_data_stream.c src/v34_data_stream.c src/v34_data_receiver.c src/v34_b1_receiver.c src/v34_data_decoder.c src/v34_b1_stream.c src/v34_qam_tx.c src/v34_trellis.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c src/v34_training_rx.c src/v34_timing.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_data_decoder: tests/test_v34_data_decoder.c src/v34_data_decoder.c src/v34_trellis.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v34_b1_receiver: tests/test_v34_b1_receiver.c src/v34_b1_receiver.c src/v34_b1_stream.c src/v34_data_decoder.c src/v34_qam_tx.c src/v34_trellis.c src/v34_data_mapper.c src/v34_mapper.c src/v34_b1.c src/v34_framing.c src/v34_training_symbols.c src/v34_training_rx.c src/v34_timing.c src/v34_caps.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

clean:
	rm -f $(OBJ) sip-softmodem tests/test_core tests/test_tones tests/test_v22_handshake tests/test_v8 tests/test_v8_fsk tests/test_v8_session tests/test_ansam tests/test_v32_std tests/test_v32bis_trellis tests/test_v32_training tests/test_v32_line tests/test_v32_rate tests/test_v32_e tests/test_v32_data tests/test_v32_qam tests/test_v32_startup tests/test_v32_startup_link tests/test_v32_retrain tests/test_v32_session tests/test_v42_hdlc tests/test_v42_lapm tests/test_v42_arq tests/test_v42_link tests/test_v42_xid tests/test_v42_session tests/test_v42_stream tests/test_v42_v32 tests/test_v34_caps tests/test_v34_info tests/test_v34_info1 tests/test_v34_phase2 tests/test_v34_timing tests/test_v34_b1 tests/test_v34_mapper tests/test_v34_data_mapper tests/test_v34_trellis tests/test_v34_b1_stream tests/test_v34_data_stream tests/test_v34_data_decoder tests/test_v34_b1_receiver tests/dsp_link
