#ifndef BIO_H_
#define BIO_H_

#include <stddef.h>

typedef unsigned char uchar;

void init();
uchar *compress(uchar *iptr, size_t isize, uchar *optr);
uchar *decompress(uchar *iptr, uchar *optr);

#endif /* BIO_H_ */
