#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <limits.h>

#define UINT32_MAX_ 4294967295U

#if (USHRT_MAX == UINT32_MAX_)
#	define UINT32 unsigned short
#elif (UINT_MAX == UINT32_MAX_)
#	define UINT32 unsigned
#elif (ULONG_MAX == UINT32_MAX_)
#	define UINT32 unsigned long
#else
#	error "Unable to find 32-bit type"
#endif

#endif /* COMMON_H_ */
