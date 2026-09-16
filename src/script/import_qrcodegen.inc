/* qrcodegen public API: 9 upstream functions + xs PNG 输出辅助层。
 * 上游 vendored 自 github.com/nayuki/QR-Code-generator（MIT）。 */

/* 编码三入口（tempBuffer/qrcode 由调用方提供，见 qrcodegen.h 缓冲说明） */
XS_QRCODEGEN_SYMBOL(qrcodegen_encodeText)
XS_QRCODEGEN_SYMBOL(qrcodegen_encodeBinary)
XS_QRCODEGEN_SYMBOL(qrcodegen_encodeSegments)
XS_QRCODEGEN_SYMBOL(qrcodegen_encodeSegmentsAdvanced)

/* 模式探测与缓冲计算 */
XS_QRCODEGEN_SYMBOL(qrcodegen_isNumeric)
XS_QRCODEGEN_SYMBOL(qrcodegen_isAlphanumeric)
XS_QRCODEGEN_SYMBOL(qrcodegen_calcSegmentBufferSize)

/* 模块矩阵读取（渲染用） */
XS_QRCODEGEN_SYMBOL(qrcodegen_getSize)
XS_QRCODEGEN_SYMBOL(qrcodegen_getModule)

/* PNG 输出（xserver 扩展辅助层，1-bit 灰度；xrtFree 释放） */
XS_QRCODEGEN_SYMBOL(qrcodegen_png)
