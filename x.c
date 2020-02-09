#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "bio.h"

void fload(void *ptr, size_t size, FILE *stream)
{
	if (fread(ptr, 1, size, stream) < size) {
		abort();
	}
}

void fsave(void *ptr, size_t size, FILE *stream)
{
	if (fwrite(ptr, 1, size, stream) < size) {
		abort();
	}
}

void fdump(void *ptr, void *end, FILE *stream)
{
	size_t size = (uchar *)end - (uchar *)ptr;

	printf("output size: %lu bytes\n", (unsigned long)size);

	fsave(ptr, size, stream);
}

size_t fsize(FILE *stream)
{
	long size;

	if (fseek(stream, 0, SEEK_END)) {
		fprintf(stderr, "Stream is not seekable\n");
		abort();
	}

	size = ftell(stream);

	if (size == (long)-1) {
		abort();
	}

	rewind(stream);

	return (size_t)size;
}

enum {
	COMPRESS,
	DECOMPRESS
};

int main(int argc, char *argv[])
{
	int mode = (argc > 0 && strstr(argv[0], "unx")) ? DECOMPRESS : COMPRESS;
	FILE *istream = argc > 1 ? fopen(argv[1], "r") : stdin;
	FILE *ostream = argc > 2 ? fopen(argv[2], "w") : stdout;
	size_t isize;
	void *iptr, *optr;
	void *end;

	fprintf(stderr, "%s\n", mode == COMPRESS ? "Compressing..." : "Decompressing...");

	if (argc == 2) {
		char path[4096];
		switch (mode) {
			case COMPRESS:
				strcpy(path, argv[1]);
				strcat(path, ".x");
				break;
			case DECOMPRESS:
				strcpy(path, argv[1]);
				if (strcmp(path + strlen(path) - 2, ".x") == 0) {
					path[strlen(path) - 2] = 0;
				} else {
					strcat(path, ".out");
				}
		}
		ostream = fopen(path, "w");
	}

	if (istream == NULL) {
		fprintf(stderr, "Cannot open input file\n");
		abort();
	}

	if (ostream == NULL) {
		fprintf(stderr, "Cannot open output file\n");
		abort();
	}

	isize = fsize(istream);

	printf("input size: %lu bytes\n", (unsigned long)isize);

	iptr = malloc(isize);
	optr = malloc(8 * isize);

	if (iptr == NULL) {
		fprintf(stderr, "Out of memory\n");
		abort();
	}

	if (optr == NULL) {
		fprintf(stderr, "Out of memory\n");
		abort();
	}

	init();

	fload(iptr, isize, istream);

	if (mode == COMPRESS) {
		end = compress(iptr, isize, optr);
	} else {
		end = decompress(iptr, optr);
	}

	fdump(optr, end, ostream);

	fclose(istream);
	fclose(ostream);

	free(iptr);
	free(optr);

	return 0;
}
