#include "bio.h"

#include <assert.h>
#include <limits.h>

#define UINT32_MAX_ 4294967295U

#if (USHRT_MAX == UINT32_MAX_)
typedef unsigned short uint32;
#elif (UINT_MAX == UINT32_MAX_)
typedef unsigned uint32;
#elif (ULONG_MAX == UINT32_MAX_)
typedef unsigned long uint32;
#else
#	error "Unable to find 32-bit type"
#endif

enum {
	BIO_MODE_READ,
	BIO_MODE_WRITE
};

struct bio {
	int mode;
	uchar *ptr;
	uint32 b; /* buffer */
	size_t c; /* counter */
};

static struct context {
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

static void bio_reset_after_flush(struct bio *bio)
{
	assert(bio != NULL);

	bio->b = 0;
	bio->c = 0;
}

static void bio_open(struct bio *bio, uchar *ptr, int mode)
{
	assert(bio != NULL);
	assert(ptr != NULL);

	bio->mode = mode;
	bio->ptr = ptr;

	switch (mode) {
		case BIO_MODE_READ:
			bio->c = 32;
			break;
		case BIO_MODE_WRITE:
			bio_reset_after_flush(bio);
			break;
	}
}

static void bio_flush_buffer(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->ptr != NULL);
	assert(sizeof(uint32) * CHAR_BIT == 32);

	*((uint32 *)bio->ptr) = bio->b;

	bio->ptr += 4;
}

static void bio_reload_buffer(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->ptr != NULL);

	bio->b = *(uint32 *)bio->ptr;

	bio->ptr += 4;
}

static void bio_put_nonzero_bit(struct bio *bio)
{
	assert(bio != NULL);
	assert(bio->c < 32);

	bio->b |= (uint32)1 << bio->c;

	bio->c++;

	if (bio->c == 32) {
		bio_flush_buffer(bio);
		bio_reset_after_flush(bio);
	}
}

static size_t minsize(size_t a, size_t b)
{
	return a < b ? a : b;
}

static size_t ctzu32(uint32 n)
{
	if (n == 0) {
		return 32;
	}

	switch (sizeof(uint32)) {
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
			return lut[((uint32)((n & -n) * 0x077CB531U)) >> 27];
	}
}

static uint32 bio_get_zeros_and_drop_bit(struct bio *bio)
{
	uint32 total_zeros = 0;

	assert(bio != NULL);

	do {
		size_t s;

		/* reload? */
		if (bio->c == 32) {
			bio_reload_buffer(bio);

			bio->c = 0;
		}

		/* get trailing zeros */
		s = minsize(32 - bio->c, ctzu32(bio->b));

		bio->b >>= s;
		bio->c += s;

		total_zeros += s;
	} while (bio->c == 32);

	assert(bio->c < 32);

	bio->b >>= 1;

	bio->c++;

	return total_zeros;
}

static void bio_write_bits(struct bio *bio, uint32 b, size_t n)
{
	assert(n <= 32);

	while (n > 0) {
		size_t m;

		assert(bio->c < 32);

		m = minsize(32 - bio->c, n);

		assert(32 >= bio->c + m);

		bio->b |= (uint32)((b & (((uint32)1 << m) - 1)) << bio->c);

		bio->c += m;

		if (bio->c == 32) {
			bio_flush_buffer(bio);
			bio_reset_after_flush(bio);
		}

		b >>= m;
		n -= m;
	}
}

static void bio_write_zero_bits(struct bio *bio, size_t n)
{
	assert(n <= 32);

	while (n > 0) {
		size_t m;

		assert(bio->c < 32);

		m = minsize(32 - bio->c, n);

		assert(32 >= bio->c + m);

		bio->c += m;

		if (bio->c == 32) {
			bio_flush_buffer(bio);
			bio_reset_after_flush(bio);
		}

		n -= m;
	}
}

static uint32 bio_read_bits(struct bio *bio, size_t n)
{
	uint32 w;
	size_t s;

	/* reload? */
	if (bio->c == 32) {
		bio_reload_buffer(bio);

		bio->c = 0;
	}

	/* get the avail. least-significant bits */
	s = minsize(32 - bio->c, n);

	w = bio->b & (((uint32)1 << s) - 1);

	bio->b >>= s;
	bio->c += s;

	n -= s;

	/* need more bits? reload & get the most-significant bits */
	if (n > 0) {
		assert(bio->c == 32);

		bio_reload_buffer(bio);

		bio->c = 0;

		w |= (bio->b & (((uint32)1 << n) - 1)) << s;

		bio->b >>= n;
		bio->c += n;
	}

	return w;
}

static void bio_close(struct bio *bio)
{
	assert(bio != NULL);

	if (bio->mode == BIO_MODE_WRITE && bio->c > 0) {
		bio_flush_buffer(bio);
	}
}

static void bio_write_unary(struct bio *bio, uint32 N)
{
	while (N > 32) {
		bio_write_zero_bits(bio, 32);

		N -= 32;
	}

	bio_write_zero_bits(bio, N);

	bio_put_nonzero_bit(bio);
}

static uint32 bio_read_unary(struct bio *bio)
{
	return bio_get_zeros_and_drop_bit(bio);
}

/* Golomb-Rice, encode non-negative integer N, parameter M = 2^k */
static void bio_write_gr(struct bio *bio, size_t k, uint32 N)
{
	uint32 Q = N >> k;

	bio_write_unary(bio, Q);

	assert(k <= 32);

	bio_write_bits(bio, N, k);
}

static uint32 bio_read_gr(struct bio *bio, size_t k)
{
	uint32 Q;
	uint32 N;

	Q = bio_read_unary(bio);

	N = Q << k;

	assert(k <= 32);

	N |= bio_read_bits(bio, k);

	return N;
}

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

static void increment_frequency(struct context *context, uchar c)
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

uchar *compress(uchar *iptr, size_t isize, uchar *optr)
{
	struct bio bio;
	uchar *end = iptr + isize;
	struct context *context = table + 0;

	bio_open(&bio, optr, BIO_MODE_WRITE);

	for (; iptr < end; ++iptr) {
		uchar c = *iptr;

		/* get index */
		uchar d = context->order[c];

		bio_write_gr(&bio, opt_k, (uint32)d);

		assert(c == context->sorted[d]);

		/* update context model */
		increment_frequency(context, c);

		/* update Golomb-Rice model */
		update_model(d);

		context = table + c;
	}

	/* EOF symbol */
	bio_write_gr(&bio, opt_k, 256);

	bio_close(&bio);

	return bio.ptr;
}

uchar *decompress(uchar *iptr, uchar *optr)
{
	struct bio bio;
	struct context *context = table + 0;

	bio_open(&bio, iptr, BIO_MODE_READ);

	do {
		uint32 d = bio_read_gr(&bio, opt_k);
		uchar c;

		if (d == 256) {
			break;
		}

		assert(d < 256);

		c = context->sorted[d];

		*(optr++) = c;

		increment_frequency(context, c);

		update_model(d);

		context = table + c;
	} while (1);

	bio_close(&bio);

	return optr;
}
