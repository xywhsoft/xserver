


// XTP 协议函数前向声明 (类型在 xtp.h 中定义)
int XTP_Send(struct mg_connection* c, str sCmd, size_t iCmdSize, 
			 uint iParamCount, str* arrParam, str* arrValue, 
			 ptr pBody, size_t iBodySize);
char* XTP_GetParam(void* msg, const char* sKey);

// XTP 协议函数注册
void ImportXTP(TCCState* s)
{
	tcc_add_symbol(s, "XTP_Send", XTP_Send);
	tcc_add_symbol(s, "XTP_GetParam", XTP_GetParam);
}

