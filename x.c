#define _POSIX_C_SOURCE 2
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include "libx.h"

void fload(void *ptr, size_t size, FILE *stream)
{
	if (fread(ptr, 1, size, stream) < size) {
		abort();
	}
}

void fsave(void *ptr, size_t size, FILE *stream)
{
	if (fwrite(ptr, 1, size, stream) < size) {
		abort();
	}
}

size_t fsize(FILE *stream)
{
	long begin = ftell(stream);

	if (begin == (long)-1) {
		abort();
	}

	if (fseek(stream, 0, SEEK_END)) {
		fprintf(stderr, "Stream is not seekable\n");
		abort();
	}

	long end = ftell(stream);

	if (end == (long)-1) {
		abort();
	}

	if (fseek(stream, begin, SEEK_SET)) {
		fprintf(stderr, "Stream is not seekable\n");
		abort();
	}

	return (size_t)end - (size_t)begin;
}

FILE *force_fopen(const char *pathname, const char *mode, int force)
{
	if (force == 0 && access(pathname, F_OK) != -1) {
		fprintf(stderr, "File already exists\n");
		abort();
	}

	return fopen(pathname, mode);
}

enum {
	COMPRESS,
	DECOMPRESS
};

void print_help(char *path)
{
	fprintf(stderr, "Usage :\n\t%s [arguments] [input-file] [output-file]\n\n", path);
	fprintf(stderr, "Arguments :\n");
	fprintf(stderr, " -1     : compress faster\n");
	fprintf(stderr, " -d     : force decompression\n");
	fprintf(stderr, " -z     : force compression\n");
	fprintf(stderr, " -f     : overwrite existing output file\n");
	fprintf(stderr, " -h     : print this message\n");
}

static size_t min_layers = 3;
static size_t max_layers = 255;

struct layer {
	void *data; /* input data */
	size_t size; /* input size */
} layer[256];

void free_layers()
{
	for (size_t j = 0; j < 256; ++j) {
		if (layer[j].data != (void *)0) {
			free(layer[j].data);
		} else {
			break;
		}
	}
}

size_t multi_compress(size_t j)
{
	assert(j + 1 < 256);

	layer[j + 1].data = malloc(8 * layer[j].size);

	if (layer[j + 1].data == NULL) {
		abort();
	}

	init();

	void *end = compress(layer[j].data, layer[j].size, layer[j + 1].data);

	layer[j + 1].size = (char *)end - (char *)layer[j + 1].data;

	if (j + 2 < 256 && j + 2 <= max_layers && (layer[j + 1].size < layer[j].size || j + 1 < min_layers)) {
		/* try next layer */
		size_t J = multi_compress(j + 1);

		if (layer[j].size < layer[J].size) {
			return j;
		} else {
			return J;
		}
	}

	if (layer[j].size < layer[j + 1].size) {
		return j;
	} else {
		return j + 1;
	}
}

void multi_decompress(size_t j)
{
	if (j == 0) {
		return;
	}

	assert(j > 0 && j < 256);

	layer[j - 1].data = malloc(8 * layer[j].size);

	if (layer[j - 1].data == NULL) {
		abort();
	}

	init();

	void *end = decompress(layer[j].data, layer[j - 1].data);

	layer[j - 1].size = (char *)end - (char *)layer[j - 1].data;

	multi_decompress(j - 1);
}

void load_layer(size_t j, FILE *stream)
{
	layer[j].size = fsize(stream);
	layer[j].data = malloc(layer[j].size);

	if (layer[j].data == NULL) {
		fprintf(stderr, "Out of memory\n");
		abort();
	}

	fload(layer[j].data, layer[j].size, stream);
}

size_t load_from_container(FILE *stream)
{
	int c = fgetc(stream);

	if (c == EOF) {
		abort();
	}

	assert(c < 256);

	size_t J = c;

	load_layer(J, stream);

	return J;
}

void save_layer(size_t j, FILE *stream)
{
	printf("Output size: %lu bytes\n", (unsigned long)layer[j].size);

	fsave(layer[j].data, layer[j].size, stream);
}

void save_to_container(size_t J, FILE *stream)
{
	if (fputc(J, stream) == EOF) {
		abort();
	}

	save_layer(J, stream);
}

int main(int argc, char *argv[])
{
	int mode = (argc > 0 && strcmp(basename(argv[0]), "unx") == 0) ? DECOMPRESS : COMPRESS;
	FILE *istream = NULL;
	FILE *ostream = NULL;
	int force = 0;

	parse: switch (getopt(argc, argv, "zdf1h")) {
		case 'z':
			mode = COMPRESS;
			goto parse;
		case 'd':
			mode = DECOMPRESS;
			goto parse;
		case 'f':
			force = 1;
			goto parse;
		case '1':
			max_layers = 1;
			goto parse;
		case 'h':
			print_help(argv[0]);
			return 0;
		default:
			abort();
		case -1:
			;
	}

	switch (argc - optind) {
		case 0:
			istream = stdin;
			ostream = stdout;
			break;
		case 1:
			istream = fopen(argv[optind], "r");
			/* guess output file name */
			if (mode == COMPRESS) {
				char path[4096];
				sprintf(path, "%s.x", argv[optind]); /* add .x suffix */
				ostream = force_fopen(path, "w", force);
			} else {
				if (strrchr(argv[optind], '.') != NULL) {
					*strrchr(argv[optind], '.') = 0; /* remove suffix */
				}
				ostream = force_fopen(argv[optind], "w", force);
			}
			break;
		case 2:
			istream = fopen(argv[optind+0], "r");
			ostream = force_fopen(argv[optind+1], "w", force);
			break;
		default:
			fprintf(stderr, "Unexpected argument\n");
			abort();
	}

	fprintf(stderr, "%s\n", mode == COMPRESS ? "Compressing..." : "Decompressing...");

	if (istream == NULL) {
		fprintf(stderr, "Cannot open input file\n");
		abort();
	}

	if (ostream == NULL) {
		fprintf(stderr, "Cannot open output file\n");
		abort();
	}

	if (mode == COMPRESS) {
		load_layer(0, istream);

		printf("Input size: %lu bytes\n", (unsigned long)layer[0].size);

		size_t J = multi_compress(0);

		printf("Number of layers: %lu\n", J);

		save_to_container(J, ostream);
	} else {
		size_t J = load_from_container(istream);

		printf("Input size: %lu bytes\n", (unsigned long)layer[J].size);

		printf("Number of layers: %lu\n", J);

		multi_decompress(J);

		save_layer(0, ostream);
	}

	free_layers();

	fclose(istream);
	fclose(ostream);

	return 0;
}
