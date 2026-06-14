# Desktop demo app — host + POT-6 + BTN-4 on the virtual bus.
#
#   make run     build and run the demo
#   make clean
#
# `cc` may be a shell function in some environments; override with e.g.
#   make CC=/usr/bin/clang run

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
ROOT    := ../..
CORE    := $(ROOT)/bbp-core
DESK    := $(ROOT)/platforms/desktop
POL     := $(ROOT)/bbp-policy
CPPFLAGS += -I$(CORE)/include -I$(DESK)/include -I$(POL)/include

LIBS    := $(POL)/libbbp-policy.a $(DESK)/libbbp-desktop.a $(CORE)/libbbp-core.a

.PHONY: build run clean libs
build: demo

libs:
	$(MAKE) -C $(CORE) CC=$(CC)
	$(MAKE) -C $(DESK) CC=$(CC)
	$(MAKE) -C $(POL)  CC=$(CC)

demo: demo.c libs
	$(CC) $(CFLAGS) $(CPPFLAGS) demo.c $(LIBS) -o demo

run: demo
	./demo

clean:
	rm -f demo
