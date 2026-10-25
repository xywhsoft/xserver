#ifndef XS_CORE_CHECKED_H
#define XS_CORE_CHECKED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Internal boundaries shared by config, archive and runtime code. */
static inline bool XS_SizeAdd(size_t a, size_t b, size_t* out)
{
	if ( out == NULL || b > SIZE_MAX - a ) return false;
	*out = a + b;
	return true;
}

static inline bool XS_U64Range(uint64_t offset, uint64_t length, uint64_t total)
{
	return offset <= total && length <= total - offset;
}

static inline bool XS_TextValid(const char* data, size_t size, size_t limit)
{
	return size <= limit && (size == 0 ||
		(data != NULL && memchr(data, '\0', size) == NULL));
}

#endif
