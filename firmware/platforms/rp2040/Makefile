# RP2040 platform — host-side unit test for the SDK-free SLOT mapping.
# The platform itself (bbp_rp2040.c) builds with the Pico SDK; see CMakeLists.txt
# and README.md. This Makefile only builds/runs what is testable on the host.
#
#   make test    build and run the SLOT-mapping test
#   make clean

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
CPPFLAGS += -Iinclude

.PHONY: test clean
test: test/test_slot
	./test/test_slot

test/test_slot: test/test_slot.c src/bbp_rp2040_slot.c include/bbp_rp2040_slot.h
	$(CC) $(CFLAGS) $(CPPFLAGS) test/test_slot.c src/bbp_rp2040_slot.c -o test/test_slot

clean:
	rm -f test/test_slot
