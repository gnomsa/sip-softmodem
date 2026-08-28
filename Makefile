CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDLIBS += -lm -lutil

SRC := src/main.c src/at.c src/pcma.c src/rtp.c src/jitter.c src/sip.c src/v21.c src/v22.c src/v22bis.c src/pty.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean test link-test
all: sip-softmodem

sip-softmodem: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

test: tests/test_core
	./tests/test_core

link-test: tests/dsp_link
	./tests/dsp_link

tests/dsp_link: tests/dsp_link.c src/v21.c src/v22.c src/v22bis.c src/pcma.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

tests/test_core: tests/test_core.c src/at.c src/pcma.c src/rtp.c src/jitter.c src/sip.c src/v21.c src/v22.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^ $(LDLIBS)

clean:
	rm -f $(OBJ) sip-softmodem tests/test_core tests/dsp_link
