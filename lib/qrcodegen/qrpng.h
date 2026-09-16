/*
 * qrpng —— 把 qrcodegen 的模块矩阵渲染为 PNG（xserver 扩展辅助层）。
 *
 * 1-bit 灰度 PNG（IHDR/IDAT/IEND）：每个模块 scale x scale 像素，
 * 四周 border 个模块的静区（规范建议 >= 4）。IDAT 用 xrt 的
 * XDEFLATE_ZLIB 流式压缩（zlib 头与 adler32 尾由 xrt 产出）；
 * PNG 每块的 CRC32 在本文件本地实现（xrt 未导出）。
 * 返回 xrtMalloc 分配的字节流（xrtFree 释放），*pOutSize 收长度。
 * 参数越界或分配/压缩失败返回 NULL。
 */
#ifndef QRPNG_H
#define QRPNG_H

#include <stddef.h>
#include <stdint.h>

/*
 * 渲染已编码的 QR（qrcodegen_encodeText/encodeBinary 的输出）为 PNG 字节流。
 *   qrcode   —— 模块矩阵（调用 getModule/getSize 之前一直有效的那块缓冲）
 *   scale    —— 每模块像素数（1..256）
 *   border   —— 四边静区模块数（0..256；规范建议 4）
 *   pOutSize —— 可空；收 PNG 字节数
 * 返回：PNG 字节流（xrtFree 释放）或 NULL。
 */
unsigned char* qrcodegen_png(const uint8_t qrcode[], int scale, int border, size_t* pOutSize);

#endif
