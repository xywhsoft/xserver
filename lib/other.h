


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


