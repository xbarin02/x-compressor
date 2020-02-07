#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

static size_t opt_k = 3;
static size_t sum_delta = 0;
static size_t N = 0;

#define RESET_INTERVAL 256

void init()
{
	int p, i, c;

	for (p = 0; p < 256; ++p) {
		for (i = 0; i < 256; ++i) {
			table[p].sorted[i] = (uchar)i;
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

static void inc_freq(struct ctx *ctx, uchar c)
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
static void update_model(uchar delta)
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

void process(uchar *ptr, size_t size, struct bio *bio)
{
	uchar *end = ptr + size;
	struct ctx *ctx = table + 0;

	for (; ptr < end; ++ptr) {
		uchar c = *ptr;

		/* get index */
		uchar d = ctx->order[c];

		bio_write_gr(bio, opt_k, (UINT32)d);

		/* update model */
		inc_freq(ctx, c);

		update_model(d);

		ctx = table + c;
	}
}

void bio_dump(struct bio *bio, void *ptr, FILE *stream)
{
	size_t size = bio->ptr - (unsigned char *)ptr;

	printf("coded stream size: %lu bytes\n", (unsigned long)size);

	if (fwrite(ptr, 1, size, stream) < size) {
		abort();
	}
}

void fload(void *ptr, size_t size, FILE *stream)
{
	if (fread(ptr, 1, size, stream) < size) {
		abort();
	}
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

int main(int argc, char *argv[])
{
	FILE *istream = fopen(argc > 1 ? argv[1] : "L", "r");
	FILE *ostream = fopen(argc > 2 ? argv[2] : "L.gr", "w");
	size_t isize;
	struct bio bio;
	void *iptr, *optr;

	if (istream == NULL) {
		abort();
	}

	if (ostream == NULL) {
		abort();
	}

	isize = fsize(istream);

	iptr = malloc(isize);
	optr = malloc(2 * isize);

	if (iptr == NULL) {
		abort();
	}

	if (optr == NULL) {
		abort();
	}

	init();

	fload(iptr, isize, istream);

	bio_open(&bio, optr, BIO_MODE_WRITE);

	process(iptr, isize, &bio);

	bio_close(&bio);
	bio_dump(&bio, optr, ostream);

	fclose(istream);
	fclose(ostream);
	free(iptr);
	free(optr);

	return 0;
}
