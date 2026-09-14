/* xserver unity TU：xmail（邮件 MIME 基础库）实现的唯一编译单元。
 *
 * XMAIL_MODULE_ALL 拉起完整特性闭包（include/xmail/features.h 展开为
 * XMAIL_FEATURE_* 与 XRT_MODULE_* 宏）；<xmail.h> 经 -Ilib/xmail/include、
 * <xrt/*.h> 垫片经 -Ilib/xmail/shim、<xrt.h> 经 -I lib 解析到声明。
 * xrt 的实现由 main.c 的 XRT_MODULE_ALL 提供，本单元只有声明。
 * 同步上游后：核对 src 清单与 import_xmail.inc（覆盖率测试兜底）。 */
#define XMAIL_MODULE_ALL 1
#include <xmail.h>

#include "src/mail/mail_core.c"
#include "src/mail/mail_charset.c"
#include "src/mail/mail_codec.c"
#include "src/mail/mail_header.c"
#include "src/mail/mail_word.c"
#include "src/mail/mail_address.c"
#include "src/mail/mail_date.c"
#include "src/mail/mail_id.c"
#include "src/mail/mail_param.c"
#include "src/mail/mail_multipart.c"
#include "src/mail/mail_message.c"
#include "src/mail/mail_tree.c"
#include "src/mail/mail_build.c"
#include "src/mail/mail_compose.c"
#include "src/transport/mail_wire.c"
#include "src/transport/mail_net.c"
#include "src/transport/mail_net_tls.c"
#include "src/transport/mail_net_deflate.c"
#include "src/transport/mail_auth.c"
