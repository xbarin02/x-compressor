#define _POSIX_C_SOURCE 2
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include "libx.h"

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

	printf("Output size: %lu bytes\n", (unsigned long)size);

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

FILE *force_fopen(const char *pathname, const char *mode, int force)
{
	if (force == 0 && access(pathname, F_OK) != -1) {
		fprintf(stderr, "File already exists\n");
		abort();
	}

	return fopen(pathname, mode);
}

enum {
	COMPRESS,
	DECOMPRESS
};

int main(int argc, char *argv[])
{
	int mode = (argc > 0 && strcmp(basename(argv[0]), "unx") == 0) ? DECOMPRESS : COMPRESS;
	FILE *istream = NULL;
	FILE *ostream = NULL;
	size_t isize;
	int force = 0;
	void *iptr, *optr, *end;

	parse: switch (getopt(argc, argv, "zdf")) {
		case 'z':
			mode = COMPRESS;
			goto parse;
		case 'd':
			mode = DECOMPRESS;
			goto parse;
		case 'f':
			force = 1;
			goto parse;
		default:
			abort();
		case -1:
			;
	}

	switch (argc - optind) {
		case 0:
			istream = stdin;
			ostream = stdout;
			break;
		case 1:
			istream = fopen(argv[optind], "r");
			break;
		case 2:
			istream = fopen(argv[optind+0], "r");
			ostream = force_fopen(argv[optind+1], "w", force);
			break;
		default:
			fprintf(stderr, "Unexpected argument\n");
			abort();
	}

	fprintf(stderr, "%s\n", mode == COMPRESS ? "Compressing..." : "Decompressing...");

	if (ostream == NULL) {
		char path[4096];
		switch (mode) {
			case COMPRESS:
				strcpy(path, argv[optind]);
				strcat(path, ".x");
				break;
			case DECOMPRESS:
				strcpy(path, argv[optind]);
				if (strlen(path) > 1 && strcmp(path + strlen(path) - 2, ".x") == 0) {
					path[strlen(path) - 2] = 0;
				} else {
					strcat(path, ".out");
				}
		}
		ostream = force_fopen(path, "w", force);
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

	printf("Input size: %lu bytes\n", (unsigned long)isize);

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
