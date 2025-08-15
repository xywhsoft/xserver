


// 获取本机 IP
char* GetLocalIP()
{
	str sRet = xCore.sNull;
	str sLocalName = malloc(260);
	if ( gethostname(sLocalName, 260) == 0 ) {
		struct hostent* host = gethostbyname(sLocalName);
		if ( host ) {
			sRet = xrtFormat("%d.%d.%d.%d", (uint8)host->h_addr_list[0][0], (uint8)host->h_addr_list[0][1], (uint8)host->h_addr_list[0][2], (uint8)host->h_addr_list[0][3]);
		}
	}
	free(sLocalName);
	return sRet;
}



// 获取本机用户名
char* GetLocalName()
{
	#if defined(_WIN32) || defined(_WIN64)
		// windows 方案
		DWORD iSize = MAX_COMPUTERNAME_LENGTH + 1;
		str sName = malloc(iSize);
		if ( GetComputerNameA(sName, &iSize) ) {
			return sName;
		} else {
			return xCore.sNull;
		}
	#else
		return xCore.sNull;
	#endif
}



// HTTP 响应（根据 mg_http_reply 修改而来，不会进行 printf 代入）
const char *mg_http_status_code_str(int status_code);
void http_reply(struct mg_connection* c, int code, const char* headers, const char* sBody, size_t iSize)
{
	if ( iSize == 0 ) {
		iSize = strlen(sBody);
	}
	mg_printf(c, "HTTP/1.1 %d %s\r\n%sContent-Length: %d\r\n\r\n", code, mg_http_status_code_str(code), headers == NULL ? "" : headers, iSize);
	mg_send(c, sBody, iSize);
	c->is_resp = 0;
}


