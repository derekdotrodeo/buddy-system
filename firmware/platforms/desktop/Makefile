# BBP desktop virtual-bus platform.
#
#   make            build libbbp-desktop.a
#   make test       build and run the integration test
#   make clean
#
# `cc` may be a shell function in some environments; override with e.g.
#   make CC=/usr/bin/clang test

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
CORE    := ../../bbp-core
CPPFLAGS += -Iinclude -I$(CORE)/include

SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
LIB     := libbbp-desktop.a
CORELIB := $(CORE)/libbbp-core.a

.PHONY: all test clean core
all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/%.o: src/%.c include/bbp_desktop.h $(CORE)/include/bbp.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Always recurse into the core build; sub-make only recompiles what changed.
core:
	$(MAKE) -C $(CORE) CC=$(CC)
$(CORELIB): core

test: test/test_vbus
	./test/test_vbus

test/test_vbus: test/test_vbus.c $(LIB) $(CORELIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB) $(CORELIB) -o $@

clean:
	rm -f $(OBJ) $(LIB) test/test_vbus
