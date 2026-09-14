/* xserver unity TU：xpop3（POP3 客户端）实现的唯一编译单元。
 *
 * XPOP3_MODULE_ALL 拉起完整特性闭包；xmail 的实现不在本单元——它由
 * lib/xmail/xmail.c 编译，共享的 transport/SASL 内部符号经
 * ../../xmail/src/internal/* 相对桥取得声明、在链接期汇合。
 * 编译 flags 需含 -Ilib/xmail/include -Ilib/xmail/shim -Ilib/xpop3/include。 */
#define XPOP3_MODULE_ALL 1
#include <xpop3.h>

#include "src/pop3/pop3.c"
#include "src/pop3/pop3_client.c"
#include "src/pop3/pop3_auth.c"
#include "src/pop3/pop3_message.c"
