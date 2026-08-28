CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDLIBS += -lm -lutil

SRC := src/main.c src/at.c src/pcma.c src/rtp.c src/jitter.c src/sip.c src/v21.c src/v22.c src/v22bis.c src/v22_handshake.c src/v32.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_rate.c src/v32_data.c src/v8.c src/v8_fsk.c src/v8_session.c src/ansam.c src/tone_detector.c src/pty.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean test link-test integration-test
all: sip-softmodem

sip-softmodem: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

test: tests/test_core tests/test_tones tests/test_v22_handshake tests/test_v8 tests/test_v8_fsk tests/test_v8_session tests/test_ansam tests/test_v32_std tests/test_v32_training tests/test_v32_line tests/test_v32_rate tests/test_v32_data
	./tests/test_core
	./tests/test_tones
	./tests/test_v22_handshake
	./tests/test_v8
	./tests/test_v8_fsk
	./tests/test_v8_session
	./tests/test_ansam
	./tests/test_v32_std
	./tests/test_v32_training
	./tests/test_v32_line
	./tests/test_v32_rate
	./tests/test_v32_data

link-test: tests/dsp_link
	./tests/dsp_link

integration-test: sip-softmodem
	python3 tests/integration_local.py

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

tests/test_v32_training: tests/test_v32_training.c src/v32_std.c src/v32_training.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_line: tests/test_v32_line.c src/v32_std.c src/v32_training.c src/v32_line.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_rate: tests/test_v32_rate.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_rate.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_v32_data: tests/test_v32_data.c src/v32_std.c src/v32_training.c src/v32_line.c src/v32_data.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

clean:
	rm -f $(OBJ) sip-softmodem tests/test_core tests/test_tones tests/test_v22_handshake tests/test_v8 tests/test_v8_fsk tests/test_v8_session tests/test_ansam tests/test_v32_std tests/test_v32_training tests/test_v32_line tests/test_v32_rate tests/test_v32_data tests/dsp_link
