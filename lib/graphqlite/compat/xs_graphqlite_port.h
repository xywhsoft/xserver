/* POSIX strndup is absent from the MinGW CRT used by XS. */
#ifndef XS_GRAPHQLITE_WINDOWS_COMPAT_H
#define XS_GRAPHQLITE_WINDOWS_COMPAT_H
#ifdef _WIN32
#include <stdlib.h>
#include <string.h>

static inline char* xs_graphqlite_strndup(const char* text, size_t limit)
{
    size_t length = strnlen(text, limit);
    if (length == (size_t)-1) return NULL;
    char* result = malloc(length + 1);
    if (result) {
        memcpy(result, text, length);
        result[length] = '\0';
    }
    return result;
}
#define strndup xs_graphqlite_strndup
#endif
#endif
