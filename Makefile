CFLAGS=-std=c99 -pedantic -Wall -Wextra -march=native
LDFLAGS=
LDLIBS=

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

.PHONY: build-pgo
build-pgo:
	$(MAKE) distclean all BUILD=profile-generate
	./x -f libx.c
	./unx -f libx.c.x libx.c.x.out
	diff libx.c libx.c.x.out
	$(MAKE) clean all BUILD=profile-use
	-$(RM) -- libx.c.x libx.c.x.out

.PHONY: check
check: all
	./x -f libx.c
	./unx -f libx.c.x libx.c.x.out
	diff libx.c libx.c.x.out
	-$(RM) -- libx.c.x libx.c.x.out
