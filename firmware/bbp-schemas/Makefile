# BBP common schemas (BS-SPEC-300) — header-only codecs + host tests.
#
#   make test    build and run the codec tests
#   make clean

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Os
CPPFLAGS += -Iinclude

.PHONY: test clean
test: test/test_schemas
	./test/test_schemas

test/test_schemas: test/test_schemas.c include/bbp_schemas.h
	$(CC) $(CFLAGS) $(CPPFLAGS) test/test_schemas.c -o test/test_schemas

clean:
	rm -f test/test_schemas
