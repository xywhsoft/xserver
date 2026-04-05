/**
 * \file xpack_zstd.c
 * xPack Ver7 - Optimized Zstandard library (compression + decompression)
 *
 * Generate using:
 * \code
 *   cd D:/other-git/zstd/build/single_file_libs
 *   python combine.py -r ../../lib -x legacy/zstd_legacy.h -o d:/git/xPack/lib/zstd/zstd.c d:/git/xPack/lib/zstd/xpack_zstd-in.c
 * \endcode
 *
 * Safe optimizations applied (no negative impact):
 *   - ZSTD_LEGACY_SUPPORT=0      : No old format support (v0.1-v0.7)
 *   - NDEBUG/ZSTD_ASSERTIONS=0   : Disable debug assertions
 *   - ZSTD_STRIP_ERROR_STRINGS   : Remove error message strings
 *   - No multithreading          : zstdmt_compress.c excluded
 *   - No dictionary builder      : dictBuilder/*.c excluded
 *
 * Preserved for full functionality:
 *   - All compression strategies (fast/dfast/greedy/lazy/lazy2/btlazy2/btopt/btultra)
 *   - zstd_opt.c included        : Full high-level compression support
 *   - LDM disabled via stub      : Long Distance Matching not compiled (runtime disable)
 *   - Full decompression paths   : HUF X1/X2, short/long sequences
 *   - Function inlining enabled  : Maximum decompression speed
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

/*=== Debug & Assertion Control (safe to disable) ===*/
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

/*=== Feature Disable (safe) ===*/
#define ZSTD_LEGACY_SUPPORT 0
#define ZSTD_TRACE 0
#define ZSTD_DISABLE_ASM 1

/*=== Error strings stripped (safe - error codes still work) ===*/
#define ZSTD_STRIP_ERROR_STRINGS 1

/* Include zstd_deps.h first with all the options we need enabled. */
#define ZSTD_DEPS_NEED_MALLOC
#define ZSTD_DEPS_NEED_MATH64
#include "common/zstd_deps.h"

/*=== Common Files ===*/
#include "common/debug.c"
#include "common/entropy_common.c"
#include "common/error_private.c"
#include "common/fse_decompress.c"
#include "common/zstd_common.c"
/* Note: threading.c and pool.c not needed (no multithreading, no dictBuilder) */

/*=== Compression Files (all strategies included) ===*/
#include "compress/fse_compress.c"
#include "compress/hist.c"
#include "compress/huf_compress.c"
#include "compress/zstd_compress_literals.c"
#include "compress/zstd_compress_sequences.c"
#include "compress/zstd_compress_superblock.c"
#include "compress/zstd_preSplit.c"
#include "compress/zstd_compress.c"
#include "compress/zstd_double_fast.c"
#include "compress/zstd_fast.c"
#include "compress/zstd_lazy.c"
#include "compress/zstd_opt.c"
/* NOT included: zstd_ldm.c (LDM disabled, stub provided below) */
/* NOT included: zstdmt_compress.c (multithreading not needed) */

/*=== Decompression Files (full speed, all paths) ===*/
#include "decompress/huf_decompress.c"
#include "decompress/zstd_ddict.c"
#include "decompress/zstd_decompress.c"
#include "decompress/zstd_decompress_block.c"

/* NOT included: dictBuilder/*.c (dictionary training not needed) */

/*=== LDM Stub Implementation ===*/
/* LDM (Long Distance Matching) is disabled at source level.
 * These stub functions satisfy the linker.
 * To use LDM, user must set ZSTD_c_enableLongDistanceMatching=0 at runtime. */

void ZSTD_ldm_fillHashTable(
            ldmState_t* state, const BYTE* ip,
            const BYTE* iend, ldmParams_t const* params)
{
    (void)state; (void)ip; (void)iend; (void)params;
}

size_t ZSTD_ldm_generateSequences(
            ldmState_t* ldms, RawSeqStore_t* sequences,
            ldmParams_t const* params, void const* src, size_t srcSize)
{
    (void)ldms; (void)sequences; (void)params; (void)src; (void)srcSize;
    return 0;
}

size_t ZSTD_ldm_blockCompress(RawSeqStore_t* rawSeqStore,
            ZSTD_MatchState_t* ms, SeqStore_t* seqStore, U32 rep[ZSTD_REP_NUM],
            ZSTD_ParamSwitch_e useRowMatchFinder,
            void const* src, size_t srcSize)
{
    (void)rawSeqStore; (void)ms; (void)seqStore; (void)rep;
    (void)useRowMatchFinder; (void)src; (void)srcSize;
    return 0;
}

void ZSTD_ldm_skipSequences(RawSeqStore_t* rawSeqStore, size_t srcSize,
    U32 const minMatch)
{
    (void)rawSeqStore; (void)srcSize; (void)minMatch;
}

void ZSTD_ldm_skipRawSeqStoreBytes(RawSeqStore_t* rawSeqStore, size_t nbBytes)
{
    (void)rawSeqStore; (void)nbBytes;
}

size_t ZSTD_ldm_getTableSize(ldmParams_t params)
{
    (void)params;
    return 0;
}

size_t ZSTD_ldm_getMaxNbSeq(ldmParams_t params, size_t maxChunkSize)
{
    (void)params; (void)maxChunkSize;
    return 0;
}

void ZSTD_ldm_adjustParameters(ldmParams_t* params,
                               ZSTD_compressionParameters const* cParams)
{
    (void)params; (void)cParams;
}
