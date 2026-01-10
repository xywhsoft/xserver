


// 动态创建 TCC 状态机（与 xs 主程序一样的配置）
// sWorkPath: 工作目录（会添加到 include 和 library 路径），可为 NULL
// 返回创建好的 TCCState，失败返回 NULL
TCCState* xsCreateTCC(const char* sWorkPath);

// 销毁 TCC 状态机
void xsDestroyTCC(TCCState* s);

// SQL 字符串转义（防止 SQL 注入）
char* sql_escape(const char* str, size_t len);

// HTTP 响应（根据 mg_http_reply 修改而来，不会进行 printf 代入）
void http_reply(struct mg_connection* c, int code, const char* headers, const char* sBody, size_t iSize);

// 解析 cookies
xdict ParseCookies(struct mg_http_message* hm);

// 释放 Cookies 表
void FreeCookies(xdict tblCookies);

// 将函数映射到 TCC 执行环境
void ImportAll(TCCState* s);


