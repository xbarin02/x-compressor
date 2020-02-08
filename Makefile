CFLAGS=-std=c89 -pedantic -Wall -Wextra -march=native
LDFLAGS=-rdynamic
LDLIBS=-lm

BIN=gr ungr

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

CFLAGS+=-flto
LDFLAGS+=-flto

.PHONY: all
all: $(BIN)

gr: gr.o bio.o

bio.o: bio.c bio.h

ungr: gr
	ln -s $< $@

.PHONY: clean
clean:
	-$(RM) -- *.o $(BIN)

.PHONY: distclean
distclean: clean
	-$(RM) -- *.gcda gmon.out cachegrind.out.* callgrind.out.*
