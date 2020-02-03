#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "bio.h"
#include "vector.h"

#if 1
#	define ADAPTIVE
#endif

typedef unsigned char uchar;

/* char -> frequency */
size_t freq[256];

/* index -> char */
uchar sorted[256];

/* char -> index */
UINT32 order[256];

void init()
{
	int i, c;

	for (i = 0; i < 256; ++i) {
		sorted[i] = (uchar)i;
	}

	for (c = 0; c < 256; ++c) {
		for (i = 0; i < 256; ++i) {
			if (sorted[i] == c) {
				order[c] = (UINT32)i;
			}
		}
	}
}

UINT32 get_index(uchar c)
{
	return order[c];
}

void swap(uchar c, uchar d)
{
	UINT32 ic = order[c];
	UINT32 id = order[d];

	assert(sorted[ic] == c);
	assert(sorted[id] == d);

	sorted[ic] = d;
	sorted[id] = c;
	order[c] = id;
	order[d] = ic;
}

void inc_freq(uchar c)
{
	UINT32 index;
	uchar d;
	freq[c]++;

	/* swap? */
retry:
	index = order[c];
	if (index > 0) {
		d = sorted[index - 1];
		if (freq[c] > freq[d]) {
			/* move c before d */
			swap(c, d);
			goto retry;
		}
	}
}

#ifdef ADAPTIVE
static size_t opt_k = 3;
static size_t sum_delta = 0;
static size_t N = 0;
#endif

#ifdef ADAPTIVE
/* https://ipnpr.jpl.nasa.gov/progress_report/42-159/159E.pdf */
void update_model(UINT32 delta)
{
	int k;

	sum_delta += delta;
	N++;

	/* mean = E{delta} = sum_delta / N */

	/* 2^k <= E{r[k]} + 0 */
	for (k = 0; (N << k) <= sum_delta; ++k)
		;

	/* printf("selected k = %i\n", k); */

	opt_k = k;
}
#endif

#ifdef ADAPTIVE
void process(FILE *istream, struct bio *bio)
#else
void process(FILE *istream, struct vector *vector)
#endif
{
	do {
		int c = fgetc(istream);
		UINT32 d;

		if (c == EOF) {
			break;
		}

		assert(c < 256);

		d = get_index(c);
#ifdef ADAPTIVE
		bio_write_gr(bio, opt_k, d);
#else
		vector_push(vector, d);
#endif

		/* update model */
		inc_freq((uchar)c);

#ifdef ADAPTIVE
		update_model(d);
#endif
	} while (1);
}

size_t vector_opt_gr_k(struct vector *vector)
{
	size_t k;
	size_t min_size = (size_t)-1;
	size_t min_size_k = 0;

	/* for each parameter */
	for (k = 0; k < 20; ++k) {
		size_t size = 0;
		UINT32 *ptr;

		for (ptr = vector->begin; ptr < vector->end; ++ptr) {
			size += bio_sizeof_gr(k, *ptr);
		}

		if (size < min_size) {
			min_size = size;
			min_size_k = k;
		}
	}

	printf("best k = %lu\n", min_size_k);

	return min_size_k;
}

int vector_to_bio(struct vector *vector, struct bio *bio)
{
	size_t k = vector_opt_gr_k(vector);
	UINT32 *ptr;

	for (ptr = vector->begin; ptr < vector->end; ++ptr) {
		bio_write_gr(bio, k, *ptr);
	}

	return 0;
}

void bio_dump(struct bio *bio, void *ptr, FILE *bstream)
{
	size_t size = bio->ptr - (unsigned char *)ptr;

	printf("coded stream size: %lu bytes\n", (unsigned long)size);

	if (fwrite(ptr, 1, size, bstream) < size) {
		abort();
	}
}

int main(int argc, char *argv[])
{
	FILE *istream = fopen(argc > 1 ? argv[1] : "L", "r");
	FILE *ostream = fopen(argc > 2 ? argv[2] : "L.gr", "w");
	struct bio bio;
	void *ptr = malloc(100000000);
#ifndef ADAPTIVE
	struct vector vector;
#endif

	if (istream == NULL) {
		abort();
	}

	if (ostream == NULL) {
		abort();
	}

	if (ptr == NULL) {
		abort();
	}

	init();

#ifndef ADAPTIVE
	vector_init(&vector, 100000000);
#endif

	bio_open(&bio, ptr, BIO_MODE_WRITE);
#ifdef ADAPTIVE
	process(istream, &bio);
	printf("opt. k = %lu\n", (unsigned long)opt_k);
#else
	process(istream, &vector);
	vector_to_bio(&vector, &bio);
#endif
	bio_close(&bio);
	bio_dump(&bio, ptr, ostream);

#ifndef ADAPTIVE
	vector_free(&vector);
#endif

	fclose(istream);
	fclose(ostream);
	free(ptr);

	return 0;
}
