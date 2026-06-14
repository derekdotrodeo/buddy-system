# BBP core — build the static library and run the host tests.
#
#   make            build libbbp-core.a
#   make test       build and run the test binary
#   make clean

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
CPPFLAGS += -Iinclude

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
LIB := libbbp-core.a

.PHONY: all test clean
all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/%.o: src/%.c include/bbp.h src/bbp_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

test: test/test_bbp
	./test/test_bbp

test/test_bbp: test/test_bbp.c $(LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB) -o $@

clean:
	rm -f $(OBJ) $(LIB) test/test_bbp
