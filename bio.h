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
	UINT32 b; /* buffer */
	size_t c; /* counter */
};

void bio_open(struct bio *bio, unsigned char *ptr, int mode);
void bio_close(struct bio *bio);

/* Golomb-Rice, encode/decode non-negative integer N, parameter M = 2^k */
void bio_write_gr(struct bio *bio, size_t k, UINT32 N);
void bio_read_gr(struct bio *bio, size_t k, UINT32 *N);

#endif /* BIO_H_ */
