#include "vector.h"

#include <stdlib.h>

int vector_init(struct vector *vector, size_t size)
{
	vector->begin = malloc(sizeof(UINT32) * size);

	if (vector->begin == NULL) {
		abort();
	}

	vector->end = vector->begin;

	return 0;
}

int vector_push(struct vector *vector, UINT32 elem)
{
	*(vector->end++) = elem;

	return 0;
}

int vector_free(struct vector *vector)
{
	free(vector->begin);

	return 0;
}
