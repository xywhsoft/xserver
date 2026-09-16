/*
 * qrpng —— qrcodegen 的 PNG 输出层（实现）。
 *
 * 依赖：宿主 xrt 的 xbuffer/xrtDeflate（XDEFLATE_ZLIB）——声明经
 * <xrt_decl.h>（无门控全量）；实现由 main.c 的 XRT_MODULE_ALL 提供。
 * CRC32 查表本地实现（xrt 未导出 crc32）。
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <xrt_decl.h>

#include "qrcodegen.h"
#include "qrpng.h"


/* ---------------- CRC32（PNG 每块校验） ---------------- */

static uint32_t s_QrCrcTable[256];
static int s_QrCrcReady = 0;

static uint32_t qrCrc32(uint32_t uCrc, const void* pData, size_t iSize)
{
    const uint8_t* pBytes = (const uint8_t*)pData;

    if ( !s_QrCrcReady ) {
        uint32_t i, j, uEntry;

        for ( i = 0; i < 256; i++ ) {
            uEntry = i;
            for ( j = 0; j < 8; j++ ) {
                uEntry = (uEntry & 1) ? (0xEDB88320u ^ (uEntry >> 1)) : (uEntry >> 1);
            }
            s_QrCrcTable[i] = uEntry;
        }
        s_QrCrcReady = 1;
    }
    uCrc = ~uCrc;
    for ( ; iSize > 0; iSize--, pBytes++ ) {
        uCrc = s_QrCrcTable[(uCrc ^ *pBytes) & 0xFF] ^ (uCrc >> 8);
    }
    return ~uCrc;
}


/* ---------------- 小工具 ---------------- */

static void qrBe32(unsigned char* pOut, uint32_t uValue)
{
    pOut[0] = (unsigned char)(uValue >> 24);
    pOut[1] = (unsigned char)(uValue >> 16);
    pOut[2] = (unsigned char)(uValue >> 8);
    pOut[3] = (unsigned char)uValue;
}

static void qrAppendChunk(xbuffer* pBuf, const char* sType,
                          const unsigned char* pData, uint32_t iSize)
{
    unsigned char aHead[8];
    unsigned char aCrc[4];
    xbytesview tView;
    uint32_t uCrc;

    qrBe32(aHead, iSize);
    memcpy(aHead + 4, sType, 4);
    tView.Data = aHead;
    tView.Size = 8;
    (void)xrtBufferAppend(pBuf, tView);

    uCrc = qrCrc32(0, aHead + 4, 4);
    if ( iSize > 0 ) {
        tView.Data = pData;
        tView.Size = iSize;
        (void)xrtBufferAppend(pBuf, tView);
        uCrc = qrCrc32(uCrc, pData, iSize);
    }
    qrBe32(aCrc, uCrc);
    tView.Data = aCrc;
    tView.Size = 4;
    (void)xrtBufferAppend(pBuf, tView);
}

/* deflate 输出回灌进 xbuffer */
static bool qrDeflateSink(xbytesview Data, ptr pData)
{
    return xrtBufferAppend((xbuffer*)pData, Data);
}


/* ---------------- 主入口 ---------------- */

unsigned char* qrcodegen_png(const uint8_t qrcode[], int scale, int border, size_t* pOutSize)
{
    static const unsigned char aSignature[8] =
        { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

    xbuffer tBuf;
    xdeflate* pDeflate;
    xdeflateconfig tConf;
    xbytesview tView;
    unsigned char* pResult = NULL;
    unsigned char* aRows = NULL;
    unsigned char aIhdr[13];
    uint32_t uCrcTemp;
    size_t iSize = 0;
    int iModules, iSide, iRowBytes, iY, iX;
    xbuffer tIdat;

    if ( qrcode == NULL || pOutSize == NULL ||
         scale < 1 || scale > 256 || border < 0 || border > 256 ) {
        return NULL;
    }
    if ( (uint64_t)(2 * border + (iModules = qrcodegen_getSize((uint8_t*)qrcode))) * scale > 0x7FFFFFFF ) {
        return NULL;  /* PNG 尺寸上限（宽高各 31 bit，这里收得更紧） */
    }
    iSide = (2 * border + iModules) * scale;
    iRowBytes = (iSide + 7) / 8;

    if ( !xrtBufferInit(&tBuf) ) {
        return NULL;
    }
    if ( !xrtBufferInit(&tIdat) ) {
        goto unit_buf;
    }

    /* 压缩前的扫描线：每行 = 滤波字节 0 + 位打包（MSB 在前；1=白，0=黑） */
    aRows = (unsigned char*)xrtMalloc((size_t)(iRowBytes + 1) * (size_t)iSide);
    if ( aRows == NULL ) {
        goto unit_all;
    }
    for ( iY = 0; iY < iSide; iY++ ) {
        unsigned char* pRow = aRows + (size_t)iY * (iRowBytes + 1);
        int iMy = iY / scale - border;

        pRow[0] = 0;  /* 滤波：None */
        memset(pRow + 1, 0xFF, (size_t)iRowBytes);
        if ( iMy < 0 || iMy >= iModules ) {
            continue;  /* 静区全白 */
        }
        for ( iX = 0; iX < iSide; iX++ ) {
            int iMx = iX / scale - border;

            if ( iMx >= 0 && iMx < iModules &&
                 qrcodegen_getModule((uint8_t*)qrcode, iMx, iMy) ) {
                pRow[1 + iX / 8] &= (unsigned char)~(0x80u >> (iX % 8));  /* 深色模块置 0 */
            }
        }
    }

    /* IDAT：XDEFLATE_ZLIB（zlib 头/adler32 尾由 xrt 产出） */
    xrtDeflateConfigInit(&tConf);
    tConf.Format = XDEFLATE_ZLIB;
    tConf.Level = 6;
    pDeflate = xrtDeflateCreate(&tConf);
    if ( pDeflate == NULL ) {
        goto unit_all;
    }
    tView.Data = aRows;
    tView.Size = (size_t)(iRowBytes + 1) * (size_t)iSide;
    if ( !xrtDeflateWrite(pDeflate, tView, XDEFLATE_FLUSH_FINISH, qrDeflateSink, (ptr)&tIdat) ) {
        xrtDeflateDestroy(pDeflate);
        goto unit_all;
    }
    xrtDeflateDestroy(pDeflate);

    /* PNG 组装 */
    tView.Data = (ptr)aSignature;
    tView.Size = 8;
    if ( !xrtBufferAppend(&tBuf, tView) ) {
        goto unit_all;
    }
    qrBe32(aIhdr + 0, (uint32_t)iSide);        /* 宽 */
    qrBe32(aIhdr + 4, (uint32_t)iSide);        /* 高 */
    aIhdr[8] = 1;                               /* 位深 1 */
    aIhdr[9] = 0;                               /* 颜色类型 0（灰度） */
    aIhdr[10] = 0;                              /* 压缩方法 */
    aIhdr[11] = 0;                              /* 滤波方法 */
    aIhdr[12] = 0;                              /* 隔行扫描：无 */
    qrAppendChunk(&tBuf, "IHDR", aIhdr, 13);
    qrAppendChunk(&tBuf, "IDAT", tIdat.Data, (uint32_t)tIdat.Size);
    qrAppendChunk(&tBuf, "IEND", NULL, 0);

    {
        bytes pTake = xrtBufferTake(&tBuf, &iSize, NULL);

        if ( pTake != NULL ) {
            pResult = pTake;
            *pOutSize = iSize;
        }
    }

unit_all:
    if ( pResult == NULL ) {
        bytes pTake = xrtBufferTake(&tBuf, &iSize, NULL);
        if ( pTake != NULL ) {
            xrtFree(pTake);
        }
    }
    {
        bytes pTake = xrtBufferTake(&tIdat, &iSize, NULL);
        if ( pTake != NULL ) {
            xrtFree(pTake);
        }
    }
    xrtBufferUnit(&tIdat);
unit_buf:
    xrtBufferUnit(&tBuf);
    if ( aRows != NULL ) {
        xrtFree(aRows);
    }
    return pResult;
}
