/* GraphQLite uses POSIX regex. TRE supplies it on Windows; libc on Linux. */
#ifdef _WIN32
#define USE_LOCAL_TRE_H
#include "../../tre/local_includes/regex.h"
#undef USE_LOCAL_TRE_H
#else
#include_next <regex.h>
#endif
