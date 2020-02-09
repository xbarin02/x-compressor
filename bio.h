/**
 * \file bio.h
 * \brief Bit input/output routines
 */
#ifndef BIO_H_
#define BIO_H_

#include <stddef.h>
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

typedef unsigned char uchar;

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

void bio_open(struct bio *bio, uchar *ptr, int mode);
void bio_close(struct bio *bio);

/* Golomb-Rice, encode/decode non-negative integer N, parameter M = 2^k */
void bio_write_gr(struct bio *bio, size_t k, uint32 N);
uint32 bio_read_gr(struct bio *bio, size_t k);

void init();
uchar *compress(uchar *iptr, size_t isize, uchar *optr);
uchar *decompress(uchar *iptr, uchar *optr);

#endif /* BIO_H_ */
