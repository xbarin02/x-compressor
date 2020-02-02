CFLAGS=-std=c89 -pedantic -Wall -Wextra -march=native -Og -g -Wno-format -D_POSIX_C_SOURCE=200112L
LDFLAGS=-rdynamic
LDLIBS=-lm

BIN=split hist

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	-$(RM) -- *.o $(BIN)
