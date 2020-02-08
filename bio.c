#include "bio.h"
#include "common.h"

#include <assert.h>
#include <limits.h>

static void bio_reset_after_flush(struct bio *bio)
{
	assert(bio != NULL);

	bio->b = 0;
	bio->c = 0;
}

void bio_open(struct bio *bio, unsigned char *ptr, int mode)
{
	assert(bio != NULL);

	bio->mode = mode;

	bio->ptr = ptr;

	assert(ptr != NULL);

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

	assert(sizeof(UINT32) * CHAR_BIT == 32);

	*((UINT32 *)bio->ptr) = bio->b;

	bio->ptr += 4;
}

static void bio_reload_buffer(struct bio *bio)
{
	assert(bio != NULL);

	assert(bio->ptr != NULL);

	bio->b = *(UINT32 *)bio->ptr;

	bio->ptr += 4;
}

static void bio_put_bit(struct bio *bio, unsigned char b)
{
	assert(bio != NULL);

	assert(bio->c < 32);

	/* do not trust the input, mask the LSB here */
	bio->b |= (UINT32)(b & 1) << bio->c;

	bio->c ++;

	if (bio->c == 32) {
		bio_flush_buffer(bio);

		bio_reset_after_flush(bio);
	}
}

static size_t size_min(size_t a, size_t b)
{
	return a < b ? a : b;
}

/* we can write at most ... bits out of 'n' */
static size_t bio_put_bits_query(struct bio *bio, size_t n)
{
	assert(bio != NULL);

	assert(bio->c < 32);

	return size_min(32 - bio->c, n);
}

/* write n bits */
static void bio_put_bits(struct bio *bio, UINT32 b, size_t n)
{
	assert(bio != NULL);

	assert(bio->c < 32);

	assert(32 >= bio->c + n);

	bio->b |= (UINT32)((b & (((UINT32)1 << n) - 1)) << bio->c);

	bio->c += n;

	if (bio->c == 32) {
		bio_flush_buffer(bio);

		bio_reset_after_flush(bio);
	}
}

static size_t ctzu32(UINT32 n)
{
	if (n == 0) {
		return 32;
	}

	switch (sizeof(UINT32)) {
		case sizeof(unsigned):
			return __builtin_ctz((unsigned)n);
		case sizeof(unsigned long):
			return __builtin_ctzl((unsigned long)n);
		default:
			/* TODO http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightModLookup */
			__builtin_trap();
	}
}

static void bio_get_zeros(struct bio *bio, UINT32 *N)
{
	/* total zeros */
	UINT32 t = 0;

	assert(bio != NULL);

	do {
		size_t s;

		/* reload */
		if (bio->c == 32) {
			bio_reload_buffer(bio);

			bio->c = 0;
		}

		/* get trailing zeros */
		s = size_min(32 - bio->c, ctzu32(bio->b));

		bio->b >>= s;
		bio->c += s;

		t += s;
	} while (bio->c == 32);

	assert(N != NULL);

	*N = t;
}

static void bio_get_bits(struct bio *bio, UINT32 *b, size_t n)
{
	UINT32 w;
	size_t s;

	/* reload? */
	if (bio->c == 32) {
		bio_reload_buffer(bio);

		bio->c = 0;
	}

	/* get the least-significant bits */
	{
		s = size_min(32 - bio->c, n); /* avail. bits */

		w = bio->b & (((UINT32)1 << s) - 1);

		bio->b >>= s;
		bio->c += s;

		n -= s;
	}

	/* need more bits? reload & get the most-significant bits */
	if (n > 0) {
		assert(bio->c == 32);

		bio_reload_buffer(bio);

		bio->c = 0;

		w |= (bio->b & (((UINT32)1 << n) - 1)) << s;

		bio->b >>= n;
		bio->c += n;
	}

	assert(b != NULL);

	*b = w;
}

/* c' = 32 - c */
static void bio_get_bit(struct bio *bio, unsigned char *b)
{
	assert(bio != NULL);

	if (bio->c == 32) {
		bio_reload_buffer(bio);

		bio->c = 0;
	}

	assert(b != NULL);

	*b = bio->b & 1;

	bio->b >>= 1;

	bio->c ++;
}

static void bio_drop_bit(struct bio *bio)
{
	assert(bio != NULL);

	assert(bio->c < 32);

	bio->b >>= 1;

	bio->c ++;
}

static void bio_write_bits(struct bio *bio, UINT32 b, size_t n)
{
	assert(n <= 32);

	for (; n > 0; ) {
		size_t m = bio_put_bits_query(bio, n);

		bio_put_bits(bio, b, m);

		b >>= m;
		n -= m;
	}
}

static void bio_read_bits(struct bio *bio, UINT32 *b, size_t n)
{
	bio_get_bits(bio, b, n);
}

void bio_close(struct bio *bio)
{
	assert(bio != NULL);

	if (bio->mode == BIO_MODE_WRITE && bio->c > 0) {
		bio_flush_buffer(bio);
	}
}

static void bio_write_unary(struct bio *bio, UINT32 N)
{
	UINT32 b = 0;

	while (N > 32) {
		bio_write_bits(bio, b, 32);

		N -= 32;
	}

	bio_write_bits(bio, b, N);

	bio_put_bit(bio, 1);
}

static void bio_read_unary(struct bio *bio, UINT32 *N)
{
	assert(N != NULL);

	bio_get_zeros(bio, N);

	bio_drop_bit(bio);
}

void bio_write_gr_1st_part(struct bio *bio, size_t k, UINT32 N)
{
	UINT32 Q = N >> k;

	bio_write_unary(bio, Q);
}

void bio_write_gr_2nd_part(struct bio *bio, size_t k, UINT32 N)
{
	assert(k <= 32);

	bio_write_bits(bio, N, k);
}

void bio_write_gr(struct bio *bio, size_t k, UINT32 N)
{
	bio_write_gr_1st_part(bio, k, N);
	bio_write_gr_2nd_part(bio, k, N);
}

void bio_read_gr_1st_part(struct bio *bio, size_t k, UINT32 *N)
{
	UINT32 Q;

	bio_read_unary(bio, &Q);

	assert(N != NULL);

	*N = Q << k;
}

void bio_read_gr_2nd_part(struct bio *bio, size_t k, UINT32 *N)
{
	UINT32 w;

	assert(k <= 32);

	bio_read_bits(bio, &w, k);

	assert(N != NULL);

	*N |= w;
}

void bio_read_gr(struct bio *bio, size_t k, UINT32 *N)
{
	bio_read_gr_1st_part(bio, k, N);
	bio_read_gr_2nd_part(bio, k, N);
}

size_t bio_sizeof_gr(size_t k, UINT32 N)
{
	size_t size;
	UINT32 Q = N >> k;

	size = Q + 1;

	size += k;

	return size;
}
