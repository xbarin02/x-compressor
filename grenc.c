#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "bio.h"

typedef unsigned char uchar;

struct {
	/* char -> frequency */
	size_t freq[256];

	/* index -> char */
	uchar sorted[256];

	/* char -> index */
	uchar order[256];
} table[256];

static size_t opt_k = 3;
static size_t sum_delta = 0;
static size_t N = 0;

#define RESET_INTERVAL 256

void init()
{
	int p, i, c;

	for (p = 0; p < 256; ++p) {
		for (i = 0; i < 256; ++i) {
			table[p].sorted[i] = (uchar)i;
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

uchar get_index(uchar p, uchar c)
{
	return table[p].order[c];
}

void swap(uchar p, uchar c, uchar d)
{
	uchar ic = table[p].order[c];
	uchar id = table[p].order[d];

	assert(table[p].sorted[ic] == c);
	assert(table[p].sorted[id] == d);

	table[p].sorted[ic] = d;
	table[p].sorted[id] = c;

	table[p].order[c] = id;
	table[p].order[d] = ic;
}

void inc_freq(uchar p, uchar c)
{
	uchar index;
	uchar d;
	table[p].freq[c]++;

	/* swap? */
retry:
	index = table[p].order[c];
	if (index > 0) {
		d = table[p].sorted[index - 1];
		if (table[p].freq[c] > table[p].freq[d]) {
			/* move c before d */
			swap(p, c, d);
			goto retry;
		}
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

void process(FILE *istream, struct bio *bio)
{
	uchar p = 0;

	do {
		int c = fgetc(istream);
		uchar d;

		if (c == EOF) {
			break;
		}

		assert(c < 256);

		d = get_index(p, c);

		bio_write_gr(bio, opt_k, (UINT32)d);

		/* update model */
		inc_freq(p, (uchar)c);

		update_model(d);

		p = c;
	} while (1);
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

	bio_open(&bio, ptr, BIO_MODE_WRITE);

	process(istream, &bio);

	bio_close(&bio);
	bio_dump(&bio, ptr, ostream);

	fclose(istream);
	fclose(ostream);
	free(ptr);

	return 0;
}
