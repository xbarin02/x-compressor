#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

size_t hist[256];

void dump_hist()
{
	int i;

	printf("# character occurences\n");

	for (i = 0; i < 256; ++i) {
		printf("%i %lu\n", i, (unsigned long)hist[i]);
	}
}

size_t dump_entropy(size_t N)
{
	size_t s = 0;
	int i;
	float H = 0;
	size_t size;

	for (i = 0; i < 256; ++i) {
		s += hist[i];
	}

	for (i = 0; i < 256; ++i) {
		float p = hist[i] / (float)s;

		if (hist[i] > 0) {
			H -= p * log2f(p);
		}
	}

	size = (size_t)(H * N / 8);

	printf("H = %f bits (%lu kbytes in total)\n", H, (unsigned long)(size / 1024));

	return size;
}

int main(int argc, char *argv[])
{
	int i;
	size_t size = 0;

	for (i = 1; i < argc; ++i) {
		FILE *stream = fopen(argv[i], "r");
		size_t N = 0;

		if (stream == NULL) {
			abort();
		}

		do {
			int c = fgetc(stream);

			if (c == EOF) {
				break;
			}

			assert(c < 256);

			hist[c]++;
			N++;
		} while (1);

		fclose(stream);

		printf("[%s] ", argv[i]);

		size += dump_entropy(N);
	}

	printf("total size = %lu kbytes\n", (unsigned long)(size / 1024));

	return 0;
}
