


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



// 解析 cookies (使用 xrt HTTP 服务器的请求结构)
xdict ParseCookies(xhttpdreq* pReq)
{
	xdict tblCookies = xrtDictCreate(sizeof(str));
	const char* sCookie = xrtHttpReqGetHeader(pReq, "cookie");
	
	if ( sCookie == NULL || sCookie[0] == '\0' ) {
		return tblCookies;
	}
	
	const char* p = sCookie;
	while ( *p ) {
		// 跳过空白
		while ( *p == ' ' || *p == '\t' ) p++;
		
		// 查找 key
		const char* pKeyStart = p;
		while ( *p && *p != '=' && *p != ';' ) p++;
		if ( *p != '=' ) break;
		size_t iKeyLen = p - pKeyStart;
		p++;  // 跳过 '='
		
		// 查找 value
		const char* pValStart = p;
		while ( *p && *p != ';' ) p++;
		size_t iValLen = p - pValStart;
		
		// 去除 value 末尾空白
		while ( iValLen > 0 && (pValStart[iValLen-1] == ' ' || pValStart[iValLen-1] == '\t') ) {
			iValLen--;
		}
		
		// 保存到字典
		if ( iKeyLen > 0 ) {
			char* sVal = xrtMalloc(iValLen + 1);
			memcpy(sVal, pValStart, iValLen);
			sVal[iValLen] = '\0';
			
			str pOldVal = NULL;
			xrtDictSetPtr(tblCookies, (ptr)pKeyStart, iKeyLen, sVal, (ptr)&pOldVal);
			if ( pOldVal ) {
				xrtFree(pOldVal);
			}
		}
		
		// 跳过分隔符
		if ( *p == ';' ) p++;
	}
	
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


