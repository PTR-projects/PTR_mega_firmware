# Host-side unit tests for the UBX parser (no ESP-IDF needed):  make -C host_test run
CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined -I../include -I..

test_ubx: test_ubx.c ../ubx_parser.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

run: test_ubx
	./test_ubx

clean:
	rm -f test_ubx
