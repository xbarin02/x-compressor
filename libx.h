#ifndef LIBX_H_
#define LIBX_H_

#include <stddef.h>

void init();
void *compress(void *iptr, size_t isize, void *optr);
void *decompress(void *iptr, size_t isize, void *optr);

#endif /* LIBX_H_ */
