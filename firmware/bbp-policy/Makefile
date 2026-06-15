# BBP policy layer (host + module).
#
#   make            build libbbp-policy.a
#   make test       build and run the integration test (over the desktop vbus)
#   make clean
#
# `cc` may be a shell function in some environments; override with e.g.
#   make CC=/usr/bin/clang test

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
CORE    := ../bbp-core
DESK    := ../platforms/desktop
CPPFLAGS += -Iinclude -I$(CORE)/include -I$(DESK)/include

SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
LIB     := libbbp-policy.a
CORELIB := $(CORE)/libbbp-core.a
DESKLIB := $(DESK)/libbbp-desktop.a

.PHONY: all test clean core desk
all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/%.o: src/%.c include/bbp_policy.h $(CORE)/include/bbp.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

core:
	$(MAKE) -C $(CORE) CC=$(CC)
desk:
	$(MAKE) -C $(DESK) CC=$(CC)
$(CORELIB): core
$(DESKLIB): desk

test: test/test_policy
	./test/test_policy

test/test_policy: test/test_policy.c $(LIB) $(CORELIB) $(DESKLIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB) $(DESKLIB) $(CORELIB) -o $@

clean:
	rm -f $(OBJ) $(LIB) test/test_policy
