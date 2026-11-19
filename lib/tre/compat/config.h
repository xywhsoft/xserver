/* Upstream Windows configuration with the MSVC-only snprintf alias disabled. */
#include "../win32/config.h"
#ifdef __GNUC__
#undef snprintf
#undef inline
#endif
