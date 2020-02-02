#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "bio.h"
#include "vector.h"

typedef unsigned char uchar;

/* char -> frequency */
size_t freq[256];

/* index -> char */
uchar sorted[256];

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

void process(FILE *istream, struct vector *vector)
{
	do {
		int c = fgetc(istream);

		if (c == EOF) {
			break;
		}

		assert(c < 256);

		vector_push(vector, get_index(c));

		/* update model */
		inc_freq((uchar)c);
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
	struct vector vector;

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

	vector_init(&vector, 100000000);

	bio_open(&bio, ptr, BIO_MODE_WRITE);
	process(istream, &vector);
	vector_to_bio(&vector, &bio);
	bio_close(&bio);
	bio_dump(&bio, ptr, ostream);

	vector_free(&vector);

	fclose(istream);
	fclose(ostream);
	free(ptr);

	return 0;
}
