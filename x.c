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
		fprintf(stderr, "Stream is not seekable\n");
		abort();
	}

	if (fseek(stream, 0, SEEK_END)) {
		abort();
	}

	long end = ftell(stream);

	if (end == (long)-1) {
		abort();
	}

	if (fseek(stream, begin, SEEK_SET)) {
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
	fprintf(stderr, " -0     : store only\n");
	fprintf(stderr, " -1     : compress faster (default)\n");
	fprintf(stderr, " -9     : compress better\n");
	fprintf(stderr, " -d     : force decompression\n");
	fprintf(stderr, " -z     : force compression\n");
	fprintf(stderr, " -f     : overwrite existing output file\n");
	fprintf(stderr, " -k     : keep (don't delete) input file (default)\n");
	fprintf(stderr, " -h     : print this message\n");
}

static size_t min_layers = 3;
static size_t max_layers = 1;

struct layer {
	void *data; /* input data */
	size_t size; /* input size */
} layer[256];

void free_layers()
{
	for (size_t j = 0; j < 256; ++j) {
		free(layer[j].data);
	}
}

size_t multi_compress(size_t j)
{
	size_t J = j + 1;

	if (J >= 256 || J > max_layers) {
		return j;
	}

	layer[J].data = malloc(8 * layer[j].size);

	if (layer[J].data == NULL) {
		abort();
	}

	init();

	void *end = compress(layer[j].data, layer[j].size, layer[J].data);

	layer[J].size = (char *)end - (char *)layer[J].data;

	if (layer[J].size < layer[j].size || J < min_layers) {
		/* try next layer */
		J = multi_compress(J);
	}

	return layer[j].size < layer[J].size ? j : J;
}

void multi_decompress(size_t j)
{
	if (j == 0) {
		return;
	}

	assert(j > 0 && j < 256);

	layer[j - 1].data = malloc(8 * layer[j].size + 4096);

	if (layer[j - 1].data == NULL) {
		abort();
	}

	init();

	void *end = decompress(layer[j].data, layer[j].size, layer[j - 1].data);

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

	fprintf(stderr, "Input size: %lu bytes\n", (unsigned long)layer[j].size);
}

size_t load_from_container(FILE *stream)
{
	int c = fgetc(stream);

	if (c == EOF) {
		fprintf(stderr, "Unexpected end of file\n");
		abort();
	}

	size_t J = c;

	load_layer(J, stream);

	return J;
}

void save_layer(size_t j, FILE *stream)
{
	fprintf(stderr, "Output size: %lu bytes\n", (unsigned long)layer[j].size);

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
	FILE *istream = NULL, *ostream = NULL;
	int force = 0;

	parse: switch (getopt(argc, argv, "zdf019kh")) {
		case 'z':
			mode = COMPRESS;
			goto parse;
		case 'd':
			mode = DECOMPRESS;
			goto parse;
		case 'f':
			force = 1;
			goto parse;
		case '0':
			max_layers = 0;
			goto parse;
		case '1':
			max_layers = 1;
			goto parse;
		case '9':
			max_layers = 255;
			goto parse;
		case 'k':
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
			istream = fopen(argv[optind + 0], "r");
			ostream = force_fopen(argv[optind + 1], "w", force);
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

		size_t J = multi_compress(0);

		fprintf(stderr, "Number of layers: %lu\n", J);

		save_to_container(J, ostream);
	} else {
		size_t J = load_from_container(istream);

		fprintf(stderr, "Number of layers: %lu\n", J);

		multi_decompress(J);

		save_layer(0, ostream);
	}

	free_layers();

	fclose(istream);
	fclose(ostream);

	return 0;
}
