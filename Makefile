CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -Isrc
LDFLAGS =

LIB_SRCS = src/esp_util.c src/esp_bits.c src/esp_huff.c src/esp_lzss.c src/esp_arc.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

.PHONY: all clean test

all: esp unesp

selftest: src/esp_selftest.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

esp: src/esp.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

unesp: esp
	ln -sf esp unesp

src/%.o: src/%.c src/esp.h src/esp_int.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o unesp esp selftest tools/test_c_engine

test: unesp esp selftest
	./selftest
	sh tests/test.sh
