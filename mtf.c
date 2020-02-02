#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar;

uchar order[256];

void init_order()
{
	int i;

	for (i = 0; i < 256; ++i) {
		order[i] = i;
	}
}

void mtf(uchar c)
{
	uchar o = order[c];
	int i;

	/* move all chars in front of 'c' one position back */
	for (i = 0; i < 256; ++i) {
		if (order[i] < o) {
			order[i]++;
		}
	}

	/* move 'c' to the front */
	order[c] = 0;
}

void process(FILE *istream, FILE *ostream)
{
	do {
		int c = getc(istream);

		if (c == EOF) {
			break;
		}

		putc(order[c], ostream);

		mtf(c);
	} while (1);
}

int main(int argc, char *argv[])
{
	FILE *istream = fopen(argc > 1 ? argv[1] : "enwik8", "r");
	FILE *ostream = fopen(argc > 2 ? argv[2] : "enwik8.mtf", "w");

	if (istream == NULL) {
		abort();
	}

	if (ostream == NULL) {
		abort();
	}

	init_order();

	process(istream, ostream);

	fclose(istream);
	fclose(ostream);

	return 0;
}
