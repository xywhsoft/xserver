


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
							if ( (c != ' ') || (c != '\t') || (c != '\r') || (c != '\n') ) {
								kl = &s[i] - k;
								m = TRUE;
							}
						}
					} else {
						break;
					}
				} else if ( c == ';' ) {
					if ( k && (kl > 0) && v ) {
						str sVal = xrtRTrim(v, &s[i] - v, " \t", 2, FALSE);
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
				str sVal = xrtRTrim(v, &s[i] - v, " \t", 2, FALSE);
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
}


