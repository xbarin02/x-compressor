CFLAGS=-std=c89 -pedantic -Wall -Wextra -march=native -D_POSIX_C_SOURCE=200112L
LDFLAGS=-rdynamic
LDLIBS=-lm

BIN=split hist mtf grenc

ifeq ($(BUILD),debug)
	CFLAGS+=-Og -g
	LDFLAGS+=-rdynamic
endif

ifeq ($(BUILD),release)
	CFLAGS+=-O3
endif

ifeq ($(BUILD),profile-generate)
	CFLAGS+=-O3 -fprofile-generate
	LDFLAGS+=-fprofile-generate
endif

ifeq ($(BUILD),profile-use)
	CFLAGS+=-O3 -fprofile-use
endif

.PHONY: all
all: $(BIN)

grenc: grenc.o bio.o

.PHONY: clean
clean:
	-$(RM) -- *.o $(BIN)

.PHONY: distclean
distclean: clean
	-$(RM) -- *.gcda
