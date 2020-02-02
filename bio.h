/**
 * \file bio.h
 * \brief Bit input/output routines
 */
#ifndef BIO_H_
#define BIO_H_

#include "common.h"

enum {
	BIO_MODE_READ,
	BIO_MODE_WRITE
};

struct bio {
	int mode;
	unsigned char *ptr;
	unsigned char b; /* buffer */
	size_t c; /* counter */
};

int bio_open(struct bio *bio, unsigned char *ptr, int mode);
int bio_close(struct bio *bio);

/* write n least-significant bits in b */
int bio_write_bits(struct bio *bio, UINT32 b, size_t n);
/* read n least-significant bits into *b */
int bio_read_bits(struct bio *bio, UINT32 *b, size_t n);

/* write a single bit */
int bio_put_bit(struct bio *bio, unsigned char b);
/* read a single bit */
int bio_get_bit(struct bio *bio, unsigned char *b);

/* Golomb-Rice, encode/decode non-negative integer N, parameter M = 2^k */
int bio_write_gr_1st_part(struct bio *bio, size_t k, UINT32 N);
int bio_write_gr_2nd_part(struct bio *bio, size_t k, UINT32 N);
int bio_read_gr_1st_part(struct bio *bio, size_t k, UINT32 *N);
int bio_read_gr_2nd_part(struct bio *bio, size_t k, UINT32 *N);

int bio_write_gr(struct bio *bio, size_t k, UINT32 N);

size_t bio_sizeof_gr(size_t k, UINT32 N);

#endif /* BIO_H_ */
