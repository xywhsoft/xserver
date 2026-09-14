/* UTF-8 path aware file IO for TCC on Windows (wide-char bridge).
 * Part of the TCC integration of XServer, LGPL-2.1-or-later; see COPYING
 * in this directory. Copyright (c) 2025-2026 xLeaves [xywhsoft].
 */
#ifndef TCC_UTF8_IO_H
#define TCC_UTF8_IO_H

#include <stdio.h>

int tcc_utf8_open(const char *path, int flags, ...);
FILE *tcc_utf8_fopen(const char *path, const char *mode);
int tcc_utf8_unlink(const char *path);

#endif
