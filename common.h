#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define UINT32_MAX_ 4294967295U
#define INT32_MAX_ 2147483647
#define INT32_MIN_ (-2147483647-1)

#if (USHRT_MAX == UINT32_MAX_)
#	define INT32 short
#	define UINT32 unsigned short
#elif (UINT_MAX == UINT32_MAX_)
#	define INT32 int
#	define UINT32 unsigned
#elif (ULONG_MAX == UINT32_MAX_)
#	define INT32 long
#	define UINT32 unsigned long
#else
#	error "Unable to find 32-bit type"
#endif

/* NOTE
 * The standard C89 does not have SIZE_MAX.
 * The (size_t)-1 is well defined in C89 under section 6.2.1.2 Signed and unsigned integers.
 */
#define SIZE_MAX_ ((size_t)-1)

#endif /* COMMON_H_ */
