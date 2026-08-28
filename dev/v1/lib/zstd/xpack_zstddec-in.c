/**
 * \file xpack_zstddec.c
 * xPack Ver7 - Minified Zstandard decompressor only
 *
 * Generate using:
 * \code
 *   cd D:/other-git/zstd/build/single_file_libs
 *   python combine.py -r ../../lib -x legacy/zstd_legacy.h -o d:/git/xPack/lib/zstd/zstddeclib.c d:/git/xPack/lib/zstd/xpack_zstddec-in.c
 * \endcode
 *
 * Optimizations applied (based on ZSTD_LIB_MINIFY):
 *   - No legacy format support (ZSTD_LEGACY_SUPPORT=0)
 *   - Decompression only (no compress/*.c)
 *   - HUF_FORCE_DECOMPRESS_X1 - single Huffman decoder
 *   - ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT - short sequence decoder only
 *   - ZSTD_STRIP_ERROR_STRINGS - remove error messages
 *   - ZSTD_NO_INLINE - reduce code size
 */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/*=== Debug & Assertion Control ===*/
#define DEBUGLEVEL 0
#define ZSTD_ASSERTIONS 0
#define NDEBUG

/*=== XXHash Configuration ===*/
#define MEM_MODULE
#undef  XXH_NAMESPACE
#define XXH_NAMESPACE ZSTD_
#undef  XXH_PRIVATE_API
#define XXH_PRIVATE_API
#undef  XXH_INLINE_ALL
#define XXH_INLINE_ALL

/*=== Feature Disable ===*/
#define ZSTD_LEGACY_SUPPORT 0
#define ZSTD_TRACE 0
#define ZSTD_DISABLE_ASM 1

/*=== ZSTD_LIB_MINIFY Optimizations ===*/
#define HUF_FORCE_DECOMPRESS_X1 1
#define ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT 1
#define ZSTD_NO_INLINE 1
#define ZSTD_STRIP_ERROR_STRINGS 1

/* Include zstd_deps.h first with all the options we need enabled. */
#define ZSTD_DEPS_NEED_MALLOC
#include "common/zstd_deps.h"

/*=== Common Files ===*/
#include "common/debug.c"
#include "common/entropy_common.c"
#include "common/error_private.c"
#include "common/fse_decompress.c"
#include "common/zstd_common.c"

/*=== Decompression Files ===*/
#include "decompress/huf_decompress.c"
#include "decompress/zstd_ddict.c"
#include "decompress/zstd_decompress.c"
#include "decompress/zstd_decompress_block.c"
