#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "bio.h"

struct context {
	/* char -> frequency */
	size_t freq[256];

	/* index -> char */
	uchar sorted[256];

	/* char -> index */
	uchar order[256];
} table[256];

static size_t opt_k;
static size_t sum_delta;
static size_t N;

#define RESET_INTERVAL 256

void init()
{
	int p, i, c;

	opt_k = 3;
	sum_delta = 0;
	N = 0;

	for (p = 0; p < 256; ++p) {
		for (i = 0; i < 256; ++i) {
			table[p].sorted[i] = (uchar)i;
			table[p].freq[i] = 0;
		}

		for (c = 0; c < 256; ++c) {
			for (i = 0; i < 256; ++i) {
				if (table[p].sorted[i] == c) {
					table[p].order[c] = (uchar)i;
				}
			}
		}
	}
}

static void swap_symbols(struct context *context, uchar c, uchar d)
{
	uchar ic;
	uchar id;

	assert(context != NULL);

	ic = context->order[c];
	id = context->order[d];

	assert(context->sorted[ic] == c);
	assert(context->sorted[id] == d);

	context->sorted[ic] = d;
	context->sorted[id] = c;

	context->order[c] = id;
	context->order[d] = ic;
}

void increment_frequency(struct context *context, uchar c)
{
	uchar d = c;
	uchar ic;
	uchar *pd;
	size_t freq_c;

	assert(context != NULL);

	ic = context->order[c];
	freq_c = ++(context->freq[c]);

	for (pd = context->sorted + ic - 1; pd >= context->sorted; --pd) {
		if (freq_c <= context->freq[*pd]) {
			break;
		}
	}

	d = *(pd + 1);

	if (c != d) {
		swap_symbols(context, c, d);
	}
}

/* https://ipnpr.jpl.nasa.gov/progress_report/42-159/159E.pdf */
void update_model(uchar delta)
{
	if (N == RESET_INTERVAL) {
		int k;

		/* mean = E{delta} = sum_delta / N */

		/* 2^k <= E{r[k]} + 0 */
		for (k = 1; (N << k) <= sum_delta; ++k)
			;

		--k;

		opt_k = k;

		N = 0;
		sum_delta = 0;
	}

	sum_delta += delta;
	N++;
}

void compress(uchar *ptr, size_t size, struct bio *bio)
{
	uchar *end = ptr + size;
	struct context *context = table + 0;

	for (; ptr < end; ++ptr) {
		uchar c = *ptr;

		/* get index */
		uchar d = context->order[c];

		bio_write_gr(bio, opt_k, (uint32)d);

		assert(c == context->sorted[d]);

		/* update context model */
		increment_frequency(context, c);

		/* update Golomb-Rice model */
		update_model(d);

		context = table + c;
	}

	/* EOF symbol */
	bio_write_gr(bio, opt_k, 256);
}

uchar *decompress(struct bio *bio, uchar *ptr)
{
	struct context *context = table + 0;

	do {
		uint32 d = bio_read_gr(bio, opt_k);
		uchar c;

		if (d == 256) {
			break;
		}

		assert(d < 256);

		c = context->sorted[d];

		*(ptr++) = c;

		increment_frequency(context, c);

		update_model(d);

		context = table + c;
	} while (1);

	return ptr;
}

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
	struct bio bio;
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
		bio_open(&bio, optr, BIO_MODE_WRITE);

		compress(iptr, isize, &bio);

		bio_close(&bio);

		end = bio.ptr;
	} else {
		bio_open(&bio, iptr, BIO_MODE_READ);

		end = decompress(&bio, optr);

		bio_close(&bio);
	}

	fdump(optr, end, ostream);

	fclose(istream);
	fclose(ostream);

	free(iptr);
	free(optr);

	return 0;
}
