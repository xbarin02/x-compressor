#ifndef VECTOR_H_
#define VECTOR_H_

#include "common.h"

struct vector {
	UINT32 *begin; /* pointer to data */
	UINT32 *end; /* supremum (occupied, not allocated) */
};

int vector_init(struct vector *vector, size_t size);
int vector_push(struct vector *vector, UINT32 elem);
int vector_free(struct vector *vector);

#endif
