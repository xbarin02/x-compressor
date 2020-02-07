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

void decompress(struct bio *bio, uchar *ptr)
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

void bio_dump(struct bio *bio, void *ptr, FILE *stream)
{
	size_t size = bio->ptr - (unsigned char *)ptr;

	printf("coded stream size: %lu bytes\n", (unsigned long)size);

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

	compress(iptr, isize, &bio);

	bio_close(&bio);
	bio_dump(&bio, optr, ostream);

#if 0
	{
		void *xptr = malloc(isize);
		FILE *xstream = fopen("L.gr.out", "w");
		init();
		bio_open(&bio, optr, BIO_MODE_READ);
		decompress(&bio, xptr);
		bio_close(&bio);
		fsave(xptr, isize, xstream);
		fclose(xstream);
		free(xptr);
	}
#endif

	fclose(istream);
	fclose(ostream);
	free(iptr);
	free(optr);

	return 0;
}
