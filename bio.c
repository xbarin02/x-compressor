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

int bio_open(struct bio *bio, unsigned char *ptr, int mode)
{
	assert(bio != NULL);

	bio->mode = mode;

	bio->ptr = ptr;

	if (ptr == NULL) {
		return 1;
	}

	switch (mode) {
		case BIO_MODE_READ:
			bio->c = CHAR_BIT;
			break;
		case BIO_MODE_WRITE:
			bio_reset_after_flush(bio);
			break;
	}

	return 0;
}

int bio_flush_buffer(struct bio *bio)
{
	assert(bio);

	if (bio->ptr == NULL) {
		return 1;
	}

	assert(CHAR_BIT == 8);

	*bio->ptr++ = bio->b;

	return 0;
}

int bio_reload_buffer(struct bio *bio)
{
	assert(bio != NULL);

	if (bio->ptr == NULL) {
		return 1;
	}

	bio->b = *bio->ptr++;

	return 0;
}

static int bio_put_bit(struct bio *bio, unsigned char b)
{
	assert(bio != NULL);

	assert(bio->c < CHAR_BIT);

	/* do not trust the input, mask the LSB here */
	bio->b |= (unsigned char)((b & 1) << bio->c);

	bio->c ++;

	if (bio->c == CHAR_BIT) {
		int err = bio_flush_buffer(bio);

		if (err) {
			return err;
		}

		bio_reset_after_flush(bio);
	}

	return 0;
}

/* c' = CHAR_BIT - c */
static int bio_get_bit(struct bio *bio, unsigned char *b)
{
	assert(bio != NULL);

	if (bio->c == CHAR_BIT) {
		int err = bio_reload_buffer(bio);

		if (err) {
			return err;
		}

		bio->c = 0;
	}

	assert(b);

	*b = bio->b & 1;

	bio->b >>= 1;

	bio->c ++;

	return 0;
}

static int bio_write_bits(struct bio *bio, UINT32 b, size_t n)
{
	size_t i;

	assert(n <= 32);

	for (i = 0; i < n; ++i) {
		/* masking the LSB omitted */
		int err = bio_put_bit(bio, (unsigned char)b);

		b >>= 1;

		if (err) {
			return err;
		}
	}

	return 0;
}

static int bio_read_bits(struct bio *bio, UINT32 *b, size_t n)
{
	size_t i;
	UINT32 word = 0;

	assert(n <= 32);

	for (i = 0; i < n; ++i) {
		unsigned char bit;

		int err = bio_get_bit(bio, &bit);

		if (err) {
			return err;
		}

		word |= (UINT32)bit << i;
	}

	assert(b);

	*b = word;

	return 0;
}

int bio_close(struct bio *bio)
{
	assert(bio != NULL);

	if (bio->mode == BIO_MODE_WRITE && bio->c > 0) {
		bio_flush_buffer(bio);
	}

	return 0;
}

int bio_write_unary(struct bio *bio, UINT32 N)
{
	UINT32 n;
	int err;

	for (n = 0; n < N; ++n) {
		int err = bio_put_bit(bio, 0);

		if (err) {
			return err;
		}
	}

	err = bio_put_bit(bio, 1);

	if (err) {
		return err;
	}

	return 0;
}

int bio_read_unary(struct bio *bio, UINT32 *N)
{
	UINT32 Q = 0;

	do {
		unsigned char b;
		int err = bio_get_bit(bio, &b);

		if (err) {
			return err;
		}

		if (b == 0)
			Q++;
		else
			break;
	} while (1);

	assert(N != NULL);

	*N = Q;

	return 0;
}

int bio_write_gr_1st_part(struct bio *bio, size_t k, UINT32 N)
{
	UINT32 Q = N >> k;

	return bio_write_unary(bio, Q);
}

int bio_write_gr_2nd_part(struct bio *bio, size_t k, UINT32 N)
{
	assert(k <= 32);

	return bio_write_bits(bio, N, k);
}

int bio_write_gr(struct bio *bio, size_t k, UINT32 N)
{
	bio_write_gr_1st_part(bio, k, N);
	bio_write_gr_2nd_part(bio, k, N);

	return 0;
}

int bio_read_gr_1st_part(struct bio *bio, size_t k, UINT32 *N)
{
	UINT32 Q;
	int err = bio_read_unary(bio, &Q);

	if (err) {
		return err;
	}

	assert(N != NULL);

	*N = Q << k;

	return 0;
}

int bio_read_gr_2nd_part(struct bio *bio, size_t k, UINT32 *N)
{
	UINT32 w;
	int err;

	assert(k <= 32);

	err = bio_read_bits(bio, &w, k);

	if (err) {
		return err;
	}

	assert(N != NULL);

	*N |= w;

	return 0;
}

size_t bio_sizeof_gr(size_t k, UINT32 N)
{
	size_t size;
	UINT32 Q = N >> k;

	size = Q + 1;

	size += k;

	return size;
}
