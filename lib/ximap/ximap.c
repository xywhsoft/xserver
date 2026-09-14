/* xserver unity TU：ximap（IMAP 客户端）实现的唯一编译单元。
 *
 * XIMAP_MODULE_ALL 拉起完整特性闭包；xmail 的实现不在本单元——它由
 * lib/xmail/xmail.c 编译，共享的 transport/SASL 内部符号经
 * ../../xmail/src/internal/* 相对桥取得声明、在链接期汇合。
 * 编译 flags 需含 -Ilib/xmail/include -Ilib/xmail/shim -Ilib/ximap/include。 */
#define XIMAP_MODULE_ALL 1
#include <ximap.h>

#include "src/imap/imap.c"
#include "src/imap/imap_data.c"
#include "src/imap/imap_body.c"
#include "src/imap/imap_client.c"
#include "src/imap/imap_auth.c"
#include "src/imap/imap_command.c"
#include "src/imap/imap_message.c"
#include "src/imap/imap_append.c"
#include "src/imap/imap_compress.c"
