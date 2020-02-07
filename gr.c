#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "bio.h"

typedef unsigned char uchar;

struct ctx {
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

static void swap(struct ctx *ctx, uchar c, uchar d)
{
	uchar ic;
	uchar id;

	assert(ctx != NULL);

	ic = ctx->order[c];
	id = ctx->order[d];

	assert(ctx->sorted[ic] == c);
	assert(ctx->sorted[id] == d);

	ctx->sorted[ic] = d;
	ctx->sorted[id] = c;

	ctx->order[c] = id;
	ctx->order[d] = ic;
}

void inc_freq(struct ctx *ctx, uchar c)
{
	uchar d = c;
	uchar ic;
	uchar *pd;
	size_t freq_c;

	assert(ctx != NULL);

	ic = ctx->order[c];
	freq_c = ++(ctx->freq[c]);

	for (pd = ctx->sorted + ic - 1; pd >= ctx->sorted; --pd) {
		if (freq_c <= ctx->freq[*pd]) {
			break;
		}
	}

	d = *(pd + 1);

	if (c != d) {
		swap(ctx, c, d);
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
	struct ctx *ctx = table + 0;

	for (; ptr < end; ++ptr) {
		uchar c = *ptr;

		/* get index */
		uchar d = ctx->order[c];

		bio_write_gr(bio, opt_k, (UINT32)d);

		assert(c == ctx->sorted[d]);

		/* update model */
		inc_freq(ctx, c);

		update_model(d);

		ctx = table + c;
	}

	/* EOF symbol */
	bio_write_gr(bio, opt_k, 256);
}

uchar *decompress(struct bio *bio, uchar *ptr)
{
	struct ctx *ctx = table + 0;

	do {
		UINT32 d;
		uchar c;

		bio_read_gr(bio, opt_k, &d);

		if (d == 256) {
			break;
		}

		assert(d < 256);

		c = ctx->sorted[d];

		*(ptr++) = c;

		inc_freq(ctx, c);

		update_model(d);

		ctx = table + c;
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

	printf("output stream size: %lu bytes\n", (unsigned long)size);

	fsave(ptr, size, stream);
}

size_t fsize(FILE *stream)
{
	long size;

	if (fseek(stream, 0, SEEK_END)) {
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
	int mode = (argc > 0 && strstr(argv[0], "ungr")) ? DECOMPRESS : COMPRESS;
	FILE *istream = argc > 1 ? fopen(argv[1], "r") : stdin;
	FILE *ostream = argc > 2 ? fopen(argv[2], "w") : stdout;
	size_t isize;
	struct bio bio;
	void *iptr, *optr;
	void *end;

	fprintf(stderr, "mode = %s (due to %s)\n", mode == COMPRESS ? "compress" : "decompress", argv[0]);

	if (argc == 2) {
		char path[4096];
		switch (mode) {
			case COMPRESS:
				strcpy(path, argv[1]);
				strcat(path, ".gr");
				break;
			case DECOMPRESS:
				strcpy(path, argv[1]);
				if (strcmp(path + strlen(path) - 3, ".gr") == 0) {
					path[strlen(path) - 3] = 0;
				} else {
					strcat(path, ".out");
				}
		}
		fprintf(stderr, "output = %s\n", path);
		ostream = fopen(path, "w");
	}

	if (istream == NULL) {
		abort();
	}

	if (ostream == NULL) {
		abort();
	}

	isize = fsize(istream);

	iptr = malloc(isize);
	optr = malloc(8 * isize);

	if (iptr == NULL) {
		abort();
	}

	if (optr == NULL) {
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
