#include "libx.h"

#include <assert.h>
#include <stdint.h>

enum {
	BIO_MODE_READ,
	BIO_MODE_WRITE
};

struct bio {
	uint32_t *ptr; /* pointer to memory */
	uint32_t b;    /* bit buffer */
	size_t c;      /* bit counter */
};

static struct context {
	size_t freq[256];          /* char -> frequency */
	unsigned char sorted[256]; /* index -> char */
	unsigned char order[256];  /* char -> index */
} table[256];

static size_t opt_k;
static size_t sum_delta, N; /* mean = sum_delta / N */

#define RESET_INTERVAL 256 /* recompute Golomb-Rice codes after... */

static void bio_open(struct bio *bio, void *ptr, int mode)
{
	assert(bio != NULL);
	assert(ptr != NULL);

	bio->ptr = ptr;

	switch (mode) {
		case BIO_MODE_READ:
			bio->c = 32;
			break;
		case BIO_MODE_WRITE:
			bio->b = 0;
			bio->c = 0;
			break;
	}
}

static void bio_flush_buffer(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->ptr != NULL);

	*(bio->ptr++) = bio->b;
	bio->b = 0;
	bio->c = 0;
}

static void bio_reload_buffer(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->ptr != NULL);

	bio->b = *(bio->ptr++);
	bio->c = 0;
}

static void bio_put_nonzero_bit(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->c < 32);

	bio->b |= (uint32_t)1 << bio->c;
	bio->c++;

	if (bio->c == 32) {
		bio_flush_buffer(bio);
	}
}

static size_t minsize(size_t a, size_t b)
{
	return a < b ? a : b;
}

static size_t ctzu32(uint32_t n)
{
	if (n == 0) {
		return 32;
	}

	switch (sizeof(uint32_t)) {
		static const int lut[32] = {
			0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
			31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
		};
#ifdef __GNUC__
		case sizeof(unsigned):
			return __builtin_ctz((unsigned)n);
		case sizeof(unsigned long):
			return __builtin_ctzl((unsigned long)n);
#endif
		default:
			/* http://graphics.stanford.edu/~seander/bithacks.html */
			return lut[((uint32_t)((n & -n) * 0x077CB531U)) >> 27];
	}
}

static void bio_write_bits(struct bio *bio, uint32_t b, size_t n)
{
	assert(n <= 32);

	for (int i = 0; i < 2; ++i) {
		assert(bio->c < 32);

		size_t m = minsize(32 - bio->c, n);

		bio->b |= (b & (((uint32_t)1 << m) - 1)) << bio->c;
		bio->c += m;

		if (bio->c == 32) {
			bio_flush_buffer(bio);
		}

		b >>= m;
		n -= m;

		if (n == 0) {
			return;
		}
	}
}

static void bio_write_zero_bits(struct bio *bio, size_t n)
{
	assert(n <= 32);

	while (n > 0) {
		assert(bio->c < 32);

		size_t m = minsize(32 - bio->c, n);

		bio->c += m;

		if (bio->c == 32) {
			bio_flush_buffer(bio);
		}

		n -= m;
	}
}

static uint32_t bio_read_bits(struct bio *bio, size_t n)
{
	if (bio->c == 32) {
		bio_reload_buffer(bio);
	}

	/* get the avail. least-significant bits */
	size_t s = minsize(32 - bio->c, n);

	uint32_t w = bio->b & (((uint32_t)1 << s) - 1);

	bio->b >>= s;
	bio->c += s;

	n -= s;

	/* need more bits? reload & get the most-significant bits */
	if (n > 0) {
		assert(bio->c == 32);

		bio_reload_buffer(bio);

		w |= (bio->b & (((uint32_t)1 << n) - 1)) << s;

		bio->b >>= n;
		bio->c += n;
	}

	return w;
}

static void bio_close(struct bio *bio, int mode)
{
	assert(bio != NULL);

	if (mode == BIO_MODE_WRITE && bio->c > 0) {
		bio_flush_buffer(bio);
	}
}

static void bio_write_unary(struct bio *bio, uint32_t N)
{
	while (N > 32) {
		bio_write_zero_bits(bio, 32);

		N -= 32;
	}

	bio_write_zero_bits(bio, N);

	bio_put_nonzero_bit(bio);
}

static uint32_t bio_read_unary(struct bio *bio)
{
	/* get zeros... */
	uint32_t total_zeros = 0;

	assert(bio != NULL);

	do {
		/* reload? */
		if (bio->c == 32) {
			bio_reload_buffer(bio);
		}

		/* get trailing zeros */
		size_t s = minsize(32 - bio->c, ctzu32(bio->b));

		bio->b >>= s;
		bio->c += s;

		total_zeros += s;
	} while (bio->c == 32);

	/* ...and drop non-zero bit */
	assert(bio->c < 32);

	bio->b >>= 1;
	bio->c++;

	return total_zeros;
}

/* Golomb-Rice, encode non-negative integer N, parameter M = 2^k */
static void bio_write_gr(struct bio *bio, size_t k, uint32_t N)
{
	uint32_t Q = N >> k;

	bio_write_unary(bio, Q);

	assert(k <= 32);

	bio_write_bits(bio, N, k);
}

static uint32_t bio_read_gr(struct bio *bio, size_t k)
{
	uint32_t Q = bio_read_unary(bio);
	uint32_t N = Q << k;

	assert(k <= 32);

	N |= bio_read_bits(bio, k);

	return N;
}

void init()
{
	opt_k = 3;
	sum_delta = 0;
	N = 0;

	for (int p = 0; p < 256; ++p) {
		for (int i = 0; i < 256; ++i) {
			table[p].sorted[i] = i;
			table[p].freq[i] = 0;
			table[p].order[i] = i;
		}
	}
}

static void swap_symbols(struct context *context, unsigned char c, unsigned char d)
{
	assert(context != NULL);

	unsigned char ic = context->order[c];
	unsigned char id = context->order[d];

	assert(context->sorted[ic] == c);
	assert(context->sorted[id] == d);

	context->sorted[ic] = d;
	context->sorted[id] = c;

	context->order[c] = id;
	context->order[d] = ic;
}

static void increment_frequency(struct context *context, unsigned char c)
{
	assert(context != NULL);

	unsigned char ic = context->order[c];
	size_t freq_c = ++(context->freq[c]);

	unsigned char *pd;

	for (pd = context->sorted + ic - 1; pd >= context->sorted; --pd) {
		if (freq_c <= context->freq[*pd]) {
			break;
		}
	}

	unsigned char d = *(pd + 1);

	if (c != d) {
		swap_symbols(context, c, d);
	}
}

/* https://ipnpr.jpl.nasa.gov/progress_report/42-159/159E.pdf */
static void update_model(unsigned char delta)
{
	if (N == RESET_INTERVAL) {
		int k;

		/* 2^k <= E{r[k]} + 0 */
		for (k = 1; (N << k) <= sum_delta; ++k)
			;

		opt_k = k - 1;

		N = 0;
		sum_delta = 0;
	}

	sum_delta += delta;
	N++;
}

void *compress(void *iptr, size_t isize, void *optr)
{
	struct bio bio;
	unsigned char *end = (unsigned char *)iptr + isize;
	struct context *context = table + 0;

	bio_open(&bio, optr, BIO_MODE_WRITE);

	for (unsigned char *iptrc = iptr; iptrc < end; ++iptrc) {
		unsigned char c = *iptrc;

		/* get index */
		unsigned char d = context->order[c];

		bio_write_gr(&bio, opt_k, (uint32_t)d);

		assert(c == context->sorted[d]);

		/* update context model */
		increment_frequency(context, c);

		/* update Golomb-Rice model */
		update_model(d);

		context = table + c;
	}

	/* EOF symbol */
	bio_write_gr(&bio, opt_k, 256);

	bio_close(&bio, BIO_MODE_WRITE);

	return bio.ptr;
}

void *decompress(void *iptr, void *optr)
{
	struct bio bio;
	struct context *context = table + 0;

	bio_open(&bio, iptr, BIO_MODE_READ);

	unsigned char *optrc = optr;

	for (;; ++optrc) {
		uint32_t d = bio_read_gr(&bio, opt_k);

		if (d >= 256) {
			break;
		}

		unsigned char c = context->sorted[d];

		*optrc = c;

		increment_frequency(context, c);

		update_model(d);

		context = table + c;
	}

	bio_close(&bio, BIO_MODE_READ);

	return optrc;
}
