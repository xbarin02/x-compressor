#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <limits.h>

#define UINT32_MAX_ 4294967295U

#if (USHRT_MAX == UINT32_MAX_)
typedef unsigned short UINT32;
#elif (UINT_MAX == UINT32_MAX_)
typedef unsigned UINT32;
#elif (ULONG_MAX == UINT32_MAX_)
typedef unsigned long UINT32;
#else
#	error "Unable to find 32-bit type"
#endif

typedef unsigned char uchar;

#endif /* COMMON_H_ */
