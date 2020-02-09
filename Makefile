CFLAGS=-std=c89 -pedantic -Wall -Wextra -march=native
LDFLAGS=-rdynamic
LDLIBS=-lm

BIN=x unx

ifeq ($(BUILD),debug)
	CFLAGS+=-Og -g
	LDFLAGS+=-rdynamic
endif

ifeq ($(BUILD),release)
	CFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(BUILD),profile-generate)
	CFLAGS+=-O3 -DNDEBUG -fprofile-generate
	LDFLAGS+=-fprofile-generate
endif

ifeq ($(BUILD),profile-use)
	CFLAGS+=-O3 -DNDEBUG -fprofile-use
endif

ifeq ($(BUILD),profile)
	CFLAGS+=-Og -g -pg
	LDFLAGS+=-rdynamic -pg
endif

.PHONY: all
all: $(BIN)

x: x.o libx.o

libx.o: libx.c libx.h

unx: x
	ln -s $< $@

.PHONY: clean
clean:
	-$(RM) -- *.o $(BIN)

.PHONY: distclean
distclean: clean
	-$(RM) -- *.gcda gmon.out cachegrind.out.* callgrind.out.*
