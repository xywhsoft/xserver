


// 函数定义
void ImportAll(TCCState* s);



// TCC 状态机错误处理回调
static void xsCreateTCC_ErrorHandler(void *opaque, const char *msg)
{
	fprintf(stderr, "[TCC] %s\n", msg);
}



// 动态创建 TCC 状态机（与 xs 主程序一样的配置）
// 返回创建好的 TCCState，失败返回 NULL
TCCState* xsCreateTCC(const char* sWorkPath)
{
	TCCState* s = tcc_new();
	if ( s == NULL ) {
		return NULL;
	}
	// 设置错误输出回调函数
	tcc_set_error_func(s, stderr, xsCreateTCC_ErrorHandler);
	// 添加系统引用文件目录
	#if defined(_WIN32) || defined(_WIN64)
		tcc_add_include_path(s, "tcc/include_win/winapi");
		tcc_add_include_path(s, "tcc/include_win");
	#else
		tcc_add_include_path(s, "tcc/include_linux");
		tcc_add_include_path(s, "/usr/include");
		tcc_add_include_path(s, "/usr/include/uapi");
		tcc_add_include_path(s, "/usr/include/asm-generic");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu/sys");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu/sys");
		tcc_add_library_path(s, "/usr/lib");
		tcc_add_library_path(s, "/usr/lib/i386-linux-gnu");
		tcc_add_library_path(s, "/usr/lib/x86_64-linux-gnu");
	#endif
	// 添加 xserver 引用文件目录
	tcc_add_include_path(s, "tcc/inc_xs");
	tcc_add_include_path(s, "tcc/include");
	tcc_add_library_path(s, "tcc/lib");
	// 添加工作目录
	if ( sWorkPath && sWorkPath[0] != '\0' ) {
		tcc_add_include_path(s, sWorkPath);
		tcc_add_library_path(s, sWorkPath);
	}
	// 设置编译到内存
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	// 导入运行时和全局数据
	ImportAll(s);
	return s;
}



// 销毁 TCC 状态机
void xsDestroyTCC(TCCState* s)
{
	if ( s ) {
		tcc_delete(s);
	}
}



// SQL 字符串转义（防止 SQL 注入）
// 注意: 推荐使用参数化查询代替字符串拼接
char* sql_escape(const char* str, size_t len)
{
	if ( str == NULL ) return xrtCopyStr("", 0);
	if ( len == 0 ) len = strlen(str);
	
	// 计算需要的缓冲区大小（最坏情况: 每个字符都需要转义）
	size_t iNeedSize = 0;
	for ( size_t i = 0; i < len; i++ ) {
		unsigned char c = (unsigned char)str[i];
		if ( c == '\0' ) {
			break;  // 遇到 NULL 字节结束
		} else if ( (c == '\'') || (c == '\\') || (c == '"') ) {
			iNeedSize += 2;
		} else if ( (c == '\r') || (c == '\n') || (c == '\t') || (c == '\b') ) {
			iNeedSize += 2;
		} else if ( c < 0x20 ) {
			// 过滤其他控制字符
			continue;
		} else {
			iNeedSize += 1;
		}
	}
	
	char* result = xrtMalloc(iNeedSize + 1);
	char* p = result;
	
	for ( size_t i = 0; i < len; i++ ) {
		unsigned char c = (unsigned char)str[i];
		if ( c == '\0' ) {
			break;  // 遇到 NULL 字节结束
		} else if ( c == '\'' ) {
			*p++ = '\'';
			*p++ = '\'';
		} else if ( c == '\\' ) {
			*p++ = '\\';
			*p++ = '\\';
		} else if ( c == '"' ) {
			*p++ = '\\';
			*p++ = '"';
		} else if ( c == '\r' ) {
			*p++ = '\\';
			*p++ = 'r';
		} else if ( c == '\n' ) {
			*p++ = '\\';
			*p++ = 'n';
		} else if ( c == '\t' ) {
			*p++ = '\\';
			*p++ = 't';
		} else if ( c == '\b' ) {
			*p++ = '\\';
			*p++ = 'b';
		} else if ( c < 0x20 ) {
			// 过滤其他控制字符
			continue;
		} else {
			*p++ = c;
		}
	}
	*p = '\0';
	return result;
}



// HTTP 响应（根据 mg_http_reply 修改而来，不会进行 printf 代入）
const char *mg_http_status_code_str(int status_code);
void http_reply(struct mg_connection* c, int code, const char* headers, const char* sBody, size_t iSize)
{
	if ( sBody == NULL ) {
		iSize = 0;
	} else if ( iSize == 0 ) {
		iSize = strlen(sBody);
	}
	mg_printf(c, "HTTP/1.1 %d %s\r\n%sContent-Length: %d\r\n\r\n", code, mg_http_status_code_str(code), headers == NULL ? "" : headers, iSize);
	if ( sBody ) {
		mg_send(c, sBody, iSize);
	}
	c->is_resp = 0;
}



// 解析 cookies
xdict ParseCookies(struct mg_http_message* hm)
{
	xdict tblCookies = xrtDictCreate(sizeof(str));
	for ( int iHdr = 0; iHdr < MG_MAX_HTTP_HEADERS; iHdr++ ) {
		if ( hm->headers[iHdr].name.len == 0 ) {
			break;
		}
		if ( xrtStrComp(hm->headers[iHdr].name.buf, "Cookie", 6, TRUE) == 0 ) {
			str s = hm->headers[iHdr].value.buf;
			int l = hm->headers[iHdr].value.len;
			str k = NULL;								// 键
			str v = NULL;								// 值
			int kl = 0;									// 键长度
			bool m = FALSE;								// 模式，设置为 TRUE 时开始采集值，FALSE 时采集键
			int i = 0;
			for ( ; i < l; i++ ) {
				uint8 c = s[i];
				if ( c == '=' ) {
					if ( k ) {
						for ( int j = i; j >= 0; j-- ) {
							c = s[j];
							if ( (c != ' ') && (c != '\t') && (c != '\r') && (c != '\n') ) {
								kl = &s[i] - k;
								m = TRUE;
							}
						}
					} else {
						break;
					}
				} else if ( c == ';' ) {
					if ( k && (kl > 0) && v ) {
						str sVal = xrtRTrim(v, &s[i] - v, " \t", 2, FALSE, NULL);
						str pOldVal = NULL;
						xrtDictSetPtr(tblCookies, k, kl, sVal, (ptr)&pOldVal);
						if ( pOldVal ) {
							printf("Duplicate definition cookie : %.*s\nOld value = %s\nNew value = %s\n", kl, k, pOldVal, sVal);
							xrtFree(pOldVal);
						}
						k = NULL;
						v = NULL;
						kl = 0;
						m = FALSE;
					} else {
						break;
					}
				} else if ( (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') ) {
					// 跳过空白字符
				} else {
					if ( m ) {
						if ( v == NULL ) {
							v = &s[i];
						}
					} else {
						if ( k == NULL ) {
							k = &s[i];
						}
					}
				}
			}
			if ( k && (kl > 0) && v ) {
				str sVal = xrtRTrim(v, &s[i] - v, " \t", 2, FALSE, NULL);
				str pOldVal = NULL;
				xrtDictSetPtr(tblCookies, k, kl, sVal, (ptr)&pOldVal);
				if ( pOldVal ) {
					printf("Duplicate definition cookie : %.*s\nOld value = %s\nNew value = %s\n", kl, k, pOldVal, sVal);
					xrtFree(pOldVal);
				}
			} else {
				printf("bad cookies : %.*s\n", l, s);
			}
		}
	}
	hm->cookies = tblCookies;
	return tblCookies;
}



// 释放 Cookies 表
bool FreeCookies_FreeProc(Dict_Key* pKey, ptr* ppVal, ptr pArg)
{
	xrtFree(*ppVal);
	return FALSE;
}
void FreeCookies(xdict tblCookies)
{
	if ( tblCookies ) {
		xrtDictWalk(tblCookies, (ptr)FreeCookies_FreeProc, NULL);
		xrtDictDestroy(tblCookies);
	}
}


