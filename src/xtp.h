


// ==================== XTP 协议定义 ====================
// XTP 是一个基于 TCP 的轻量级二进制协议
// 报文结构: [Header 16B] + [ParamInfo 4B*n] + [Cmd] + [Params] + [Body]



// XTP 报文头结构 (16 字节)
typedef struct {
	unsigned char HeadInfo[4];      // 包头标识，固定为 "xtp\1"
	unsigned int PackSize;          // 包总长度 (包含头部)
	unsigned short CmdSize;         // 命令文本长度
	unsigned short ParamCount;      // 参数数量
	unsigned int BodySize;          // Body 长度
} XTP_PackHeader;

// XTP 参数信息结构 (4 字节)
typedef struct {
	unsigned short KeySize;         // 参数名长度
	unsigned short ValSize;         // 参数值长度
} XTP_ParamInfo;

// XTP 消息结构 (传递给 EventProc 回调)
typedef struct {
	char* Cmd;                      // 命令名 (以 \0 结尾)
	unsigned short CmdSize;         // 命令名长度
	xdict Params;                   // 参数字典 (key -> value)
	unsigned short ParamCount;      // 参数数量
	char* Body;                     // Body 数据 (以 \0 结尾)
	unsigned int BodySize;          // Body 长度
} XTP_Message, *XTP_MessageObject;



// ==================== XTP 协议解析 ====================

// 参数字典释放回调
static int XTP_FreeParamProc(ptr pKey, ptr pVal, ptr pArg)
{
	if ( pVal ) {
		xrtFree(pVal);
	}
	return 0;
}

// 解析 XTP 协议包
// 返回值: 1=成功处理一个包, 0=数据不足, -1=协议错误
static int XTP_ProcessPacket(struct mg_connection *c, XS_ServerObject objServer)
{
	// 最小封包 16 字节
	if ( c->recv.len < 16 ) {
		return 0;
	}
	
	// 检查包头标识 [xtp\1]
	if ( (c->recv.buf[0] != 'x') || (c->recv.buf[1] != 't') || 
		 (c->recv.buf[2] != 'p') || (c->recv.buf[3] != 1) ) {
		MG_ERROR(("XTP: Invalid packet header"));
		c->recv.len = 0;
		return -1;
	}
	
	// 解析包头
	XTP_PackHeader* pHeader = (XTP_PackHeader*)c->recv.buf;
	
	// 检查包是否完整
	if ( pHeader->PackSize > c->recv.len ) {
		return 0;
	}
	
	// 安全性检查：防止整数溢出和越界访问
	size_t iMinPackSize = 16 + ((size_t)pHeader->ParamCount * 4) + pHeader->CmdSize + pHeader->BodySize;
	if ( iMinPackSize > pHeader->PackSize || pHeader->PackSize > c->recv.len ) {
		MG_ERROR(("XTP: Packet size mismatch or overflow"));
		c->recv.len = 0;
		return -1;
	}
	
	// 计算各部分偏移
	unsigned int iPos = 16 + (4 * pHeader->ParamCount);
	
	// 检查命令读取越界
	if ( iPos + pHeader->CmdSize > pHeader->PackSize ) {
		MG_ERROR(("XTP: Cmd read overflow"));
		c->recv.len = 0;
		return -1;
	}
	
	// 提取命令 (添加 \0 结尾)
	char* sCmd = xrtMalloc(pHeader->CmdSize + 1);
	memcpy(sCmd, &c->recv.buf[iPos], pHeader->CmdSize);
	sCmd[pHeader->CmdSize] = '\0';
	iPos += pHeader->CmdSize;
	
	// 解析参数
	xdict tblParams = NULL;
	if ( pHeader->ParamCount > 0 ) {
		tblParams = xrtDictCreate(0);
		for ( int i = 0; i < pHeader->ParamCount; i++ ) {
			XTP_ParamInfo* pParam = (XTP_ParamInfo*)&c->recv.buf[16 + (4 * i)];
			
			// 检查参数读取越界
			if ( iPos + pParam->KeySize + pParam->ValSize > pHeader->PackSize ) {
				MG_ERROR(("XTP: Param read overflow"));
				xrtFree(sCmd);
				xrtDictWalk(tblParams, (ptr)XTP_FreeParamProc, NULL);
				xrtDictDestroy(tblParams);
				c->recv.len = 0;
				return -1;
			}
			
			char* sKey = &c->recv.buf[iPos];
			iPos += pParam->KeySize;
			
			// 复制参数值 (添加 \0 结尾)
			char* sVal = xrtMalloc(pParam->ValSize + 1);
			memcpy(sVal, &c->recv.buf[iPos], pParam->ValSize);
			sVal[pParam->ValSize] = '\0';
			iPos += pParam->ValSize;
			
			// 添加到字典
			xrtDictSetPtr(tblParams, sKey, pParam->KeySize, sVal, NULL);
		}
	}
	
	// 检查 Body 读取越界
	if ( iPos + pHeader->BodySize > pHeader->PackSize ) {
		MG_ERROR(("XTP: Body read overflow"));
		xrtFree(sCmd);
		if ( tblParams ) {
			xrtDictWalk(tblParams, (ptr)XTP_FreeParamProc, NULL);
			xrtDictDestroy(tblParams);
		}
		c->recv.len = 0;
		return -1;
	}
	
	// 提取 Body (添加 \0 结尾)
	char* sBody = xrtMalloc(pHeader->BodySize + 1);
	memcpy(sBody, &c->recv.buf[iPos], pHeader->BodySize);
	sBody[pHeader->BodySize] = '\0';
	
	// 构建消息结构
	XTP_Message msg = {
		.Cmd = sCmd,
		.CmdSize = pHeader->CmdSize,
		.Params = tblParams,
		.ParamCount = pHeader->ParamCount,
		.Body = sBody,
		.BodySize = pHeader->BodySize
	};
	
	// 调用脚本的 EventProc 回调处理 XTP 消息
	if ( objServer && objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, 
						  struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, c, MG_EV_READ, &msg);
	}
	
	// 释放内存
	xrtFree(sCmd);
	xrtFree(sBody);
	if ( tblParams ) {
		// 释放字典中的值
		xrtDictWalk(tblParams, (ptr)XTP_FreeParamProc, NULL);
		xrtDictDestroy(tblParams);
	}
	
	// 从缓冲区删除已处理的数据
	mg_iobuf_del(&c->recv, 0, pHeader->PackSize);
	
	return 1;
}



// ==================== XTP 发送函数 ====================

// 发送 XTP 协议包
// 参数:
//   c           - 连接对象
//   sCmd        - 命令名
//   iCmdSize    - 命令名长度 (0 表示自动计算)
//   iParamCount - 参数数量
//   arrParam    - 参数名数组
//   arrValue    - 参数值数组
//   pBody       - Body 数据
//   iBodySize   - Body 长度 (0 表示自动计算字符串长度)
int XTP_Send(struct mg_connection* c, str sCmd, size_t iCmdSize, 
			 uint iParamCount, str* arrParam, str* arrValue, 
			 ptr pBody, size_t iBodySize)
{
	if ( c == NULL || sCmd == NULL ) return FALSE;
	
	// 自动计算长度
	if ( iCmdSize == 0 ) {
		iCmdSize = strlen(sCmd);
	}
	if ( pBody && (iBodySize == 0) ) {
		iBodySize = strlen(pBody);
	}
	
	// 检查参数数量上限（防止整数溢出）
	if ( iParamCount > 65535 ) {
		MG_ERROR(("XTP_Send: Too many parameters"));
		return FALSE;
	}
	
	// 使用 size_t 计算包总长度，防止整数溢出
	size_t iPackSize = 16 + ((size_t)iParamCount * 4) + iCmdSize + iBodySize;
	
	// 构建参数信息列表
	XTP_ParamInfo* pParamInfo = NULL;
	if ( iParamCount > 0 ) {
		pParamInfo = xrtMalloc(iParamCount * sizeof(XTP_ParamInfo));
		for ( uint i = 0; i < iParamCount; i++ ) {
			size_t keyLen = strlen(arrParam[i]);
			size_t valLen = strlen(arrValue[i]);
			// 检查单个参数长度上限
			if ( keyLen > 65535 || valLen > 65535 ) {
				MG_ERROR(("XTP_Send: Parameter too long"));
				xrtFree(pParamInfo);
				return FALSE;
			}
			pParamInfo[i].KeySize = (unsigned short)keyLen;
			pParamInfo[i].ValSize = (unsigned short)valLen;
			iPackSize += keyLen + valLen;
		}
	}
	
	// 检查包总大小上限 (4GB)
	if ( iPackSize > 0xFFFFFFFF ) {
		MG_ERROR(("XTP_Send: Packet too large"));
		if ( pParamInfo ) xrtFree(pParamInfo);
		return FALSE;
	}
	
	// 发送包头
	XTP_PackHeader header = {
		.HeadInfo = {'x', 't', 'p', 1},
		.PackSize = (unsigned int)iPackSize,
		.CmdSize = (unsigned short)iCmdSize,
		.ParamCount = (unsigned short)iParamCount,
		.BodySize = (unsigned int)iBodySize
	};
	mg_send(c, &header, sizeof(header));
	
	// 发送参数信息头
	if ( iParamCount > 0 && pParamInfo ) {
		mg_send(c, pParamInfo, iParamCount * sizeof(XTP_ParamInfo));
	}
	
	// 发送命令
	if ( iCmdSize > 0 ) {
		mg_send(c, sCmd, iCmdSize);
	}
	
	// 发送参数键值对
	for ( uint i = 0; i < iParamCount; i++ ) {
		if ( pParamInfo[i].KeySize > 0 ) {
			mg_send(c, arrParam[i], pParamInfo[i].KeySize);
		}
		if ( pParamInfo[i].ValSize > 0 ) {
			mg_send(c, arrValue[i], pParamInfo[i].ValSize);
		}
	}
	
	// 发送 Body
	if ( iBodySize > 0 && pBody ) {
		mg_send(c, pBody, iBodySize);
	}
	
	// 释放参数信息
	if ( pParamInfo ) {
		xrtFree(pParamInfo);
	}
	
	return TRUE;
}



// 获取 XTP 消息的参数值
char* XTP_GetParam(void* pMsg, const char* sKey)
{
	XTP_MessageObject msg = (XTP_MessageObject)pMsg;
	if ( msg == NULL || msg->Params == NULL || sKey == NULL ) return NULL;
	return xrtDictGetPtr(msg->Params, (ptr)sKey, strlen(sKey));
}



// ==================== XTP 协议处理 ====================

// XTP 协议事件处理
static void ProcXTP(struct mg_connection *c, int ev, void *ev_data)
{
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	
	if ( ev == MG_EV_OPEN ) {
		// 连接打开
	} else if ( ev == MG_EV_ACCEPT ) {
		// 接受新连接
		if ( objServer && objServer->DefaultHost.EventProc ) {
			void (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, 
							  struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data);
		}
	} else if ( ev == MG_EV_READ ) {
		// 处理所有完整的数据包
		while ( XTP_ProcessPacket(c, objServer) > 0 ) {
			// 继续处理下一个包
		}
	} else if ( ev == MG_EV_CLOSE ) {
		// 连接关闭
		if ( objServer && objServer->DefaultHost.EventProc ) {
			void (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, 
							  struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data);
		}
	} else if ( ev == MG_EV_ERROR ) {
		// 错误
		MG_ERROR(("XTP error: %s", (char*)ev_data));
	}
}



// XTPS (XTP over TLS) 协议事件处理
static void ProcXTPS(struct mg_connection *c, int ev, void *ev_data)
{
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	
	// 设置 TLS
	if ( ev == MG_EV_ACCEPT ) {
		if ( objServer && objServer->EnableDefaultHost ) {
			InitTLS(c, &objServer->DefaultHost, objServer);
		} else {
			MG_ERROR(("XTPS: No default host for TLS"));
			return;
		}
	}
	
	// 调用普通 XTP 处理
	ProcXTP(c, ev, ev_data);
}



// ==================== XTP 服务启动/停止 ====================

// 启动 XTP 服务
int RunServerXTP(XS_ServerObject objServer)
{
	// 启动 XTP 服务
	printf("\n    Run Server [XTP] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_listen(&mgr, objServer->Addr, ProcXTP, objServer);
	if ( objConn == NULL ) {
		printf("    !!! ERROR !!! Cannot listen on %s. Use tcp://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	
	// 启动 XTPS 服务 (如果启用 TLS)
	if ( objServer->EnableTLS ) {
		printf("    Run Server [XTPS] : %s (%s)\n", objServer->Name, objServer->AddrTLS);
		objConn = mg_listen(&mgr, objServer->AddrTLS, ProcXTPS, objServer);
		if ( objConn == NULL ) {
			printf("    !!! ERROR !!! Cannot listen on %s. Use tcp://ADDR:PORT or :PORT\n", objServer->AddrTLS);
			exit(EXIT_FAILURE);
		} else {
			objServer->ConnTLS = objConn;
		}
	}
	
	return TRUE;
}



// 停止 XTP 服务
int StopServerXTP(XS_ServerObject objServer)
{
	// XTP 不需要特殊清理，连接由 mongoose 管理
	return TRUE;
}


