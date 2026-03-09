


// ==================== XTP 协议定义 ====================
// XTP 是一个基于 TCP 的轻量级二进制协议
// 报文结构: [Header 16B] + [ParamInfo 4B*n] + [Cmd] + [Params] + [Body]
// 基于 xrt TCP 服务器 (xtcpserver) 实现



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

// XTP 连接上下文 (用于粘包处理)
typedef struct {
	xnetbuf tRecvBuf;               // 接收缓冲区
	bool bInited;                   // 是否已初始化
} XTP_ConnContext;

// XTP 连接上下文表 (按连接 ID 索引)
static XTP_ConnContext* g_arrXtpCtx = NULL;
static int g_iXtpMaxClients = 0;



// ==================== XTP 协议解析 ====================

// 参数字典释放回调
static int XTP_FreeParamProc(ptr pKey, ptr pVal, ptr pArg)
{
	if ( pVal ) {
		xrtFree(pVal);
	}
	return 0;
}

// 获取连接上下文
static XTP_ConnContext* XTP_GetContext(int iClientId)
{
	if ( iClientId < 0 || iClientId >= g_iXtpMaxClients || g_arrXtpCtx == NULL ) {
		return NULL;
	}
	return &g_arrXtpCtx[iClientId];
}

// 初始化连接上下文
static void XTP_InitContext(int iClientId)
{
	XTP_ConnContext* pCtx = XTP_GetContext(iClientId);
	if ( pCtx && !pCtx->bInited ) {
		xrtNetBufInit(&pCtx->tRecvBuf, 8192);
		pCtx->bInited = true;
	}
}

// 清理连接上下文
static void XTP_CleanupContext(int iClientId)
{
	XTP_ConnContext* pCtx = XTP_GetContext(iClientId);
	if ( pCtx && pCtx->bInited ) {
		xrtNetBufFree(&pCtx->tRecvBuf);
		pCtx->bInited = false;
	}
}

// 解析 XTP 协议包
// 返回值: 1=成功处理一个包, 0=数据不足, -1=协议错误
static int XTP_ProcessPacket(XS_ServerObject objServer, xnetconn* pConn, XTP_ConnContext* pCtx)
{
	xnetbuf* pRecvBuf = &pCtx->tRecvBuf;
	
	// 最小封包 16 字节
	if ( pRecvBuf->iSize < 16 ) {
		return 0;
	}
	
	// 检查包头标识 [xtp\1]
	if ( (pRecvBuf->pData[0] != 'x') || (pRecvBuf->pData[1] != 't') || 
		 (pRecvBuf->pData[2] != 'p') || (pRecvBuf->pData[3] != 1) ) {
		printf("XTP: Invalid packet header\n");
		xrtNetBufConsume(pRecvBuf, pRecvBuf->iSize);
		return -1;
	}
	
	// 解析包头
	XTP_PackHeader* pHeader = (XTP_PackHeader*)pRecvBuf->pData;
	
	// 检查包是否完整
	if ( pHeader->PackSize > pRecvBuf->iSize ) {
		return 0;
	}
	
	// 安全性检查：防止整数溢出和越界访问
	size_t iMinPackSize = 16 + ((size_t)pHeader->ParamCount * 4) + pHeader->CmdSize + pHeader->BodySize;
	if ( iMinPackSize > pHeader->PackSize || pHeader->PackSize > pRecvBuf->iSize ) {
		printf("XTP: Packet size mismatch or overflow\n");
		xrtNetBufConsume(pRecvBuf, pRecvBuf->iSize);
		return -1;
	}
	
	// 计算各部分偏移
	unsigned int iPos = 16 + (4 * pHeader->ParamCount);
	
	// 检查命令读取越界
	if ( iPos + pHeader->CmdSize > pHeader->PackSize ) {
		printf("XTP: Cmd read overflow\n");
		xrtNetBufConsume(pRecvBuf, pRecvBuf->iSize);
		return -1;
	}
	
	// 提取命令 (添加 \0 结尾)
	char* sCmd = xrtMalloc(pHeader->CmdSize + 1);
	memcpy(sCmd, &pRecvBuf->pData[iPos], pHeader->CmdSize);
	sCmd[pHeader->CmdSize] = '\0';
	iPos += pHeader->CmdSize;
	
	// 解析参数
	xdict tblParams = NULL;
	if ( pHeader->ParamCount > 0 ) {
		tblParams = xrtDictCreate(0);
		for ( int i = 0; i < pHeader->ParamCount; i++ ) {
			XTP_ParamInfo* pParam = (XTP_ParamInfo*)&pRecvBuf->pData[16 + (4 * i)];
			
			// 检查参数读取越界
			if ( iPos + pParam->KeySize + pParam->ValSize > pHeader->PackSize ) {
				printf("XTP: Param read overflow\n");
				xrtFree(sCmd);
				xrtDictWalk(tblParams, (ptr)XTP_FreeParamProc, NULL);
				xrtDictDestroy(tblParams);
				xrtNetBufConsume(pRecvBuf, pRecvBuf->iSize);
				return -1;
			}
			
			char* sKey = &pRecvBuf->pData[iPos];
			iPos += pParam->KeySize;
			
			// 复制参数值 (添加 \0 结尾)
			char* sVal = xrtMalloc(pParam->ValSize + 1);
			memcpy(sVal, &pRecvBuf->pData[iPos], pParam->ValSize);
			sVal[pParam->ValSize] = '\0';
			iPos += pParam->ValSize;
			
			// 添加到字典
			xrtDictSetPtr(tblParams, sKey, pParam->KeySize, sVal, NULL);
		}
	}
	
	// 检查 Body 读取越界
	if ( iPos + pHeader->BodySize > pHeader->PackSize ) {
		printf("XTP: Body read overflow\n");
		xrtFree(sCmd);
		if ( tblParams ) {
			xrtDictWalk(tblParams, (ptr)XTP_FreeParamProc, NULL);
			xrtDictDestroy(tblParams);
		}
		xrtNetBufConsume(pRecvBuf, pRecvBuf->iSize);
		return -1;
	}
	
	// 提取 Body (添加 \0 结尾)
	char* sBody = xrtMalloc(pHeader->BodySize + 1);
	memcpy(sBody, &pRecvBuf->pData[iPos], pHeader->BodySize);
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
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_RECV, (const char*)&msg, sizeof(XTP_Message));
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
	xrtNetBufConsume(pRecvBuf, pHeader->PackSize);
	
	return 1;
}



// ==================== XTP 发送函数 ====================

// 发送 XTP 协议包
// 参数:
//   pServer     - xrt TCP 服务器
//   iClientId   - 客户端 ID
//   sCmd        - 命令名
//   iCmdSize    - 命令名长度 (0 表示自动计算)
//   iParamCount - 参数数量
//   arrParam    - 参数名数组
//   arrValue    - 参数值数组
//   pBody       - Body 数据
//   iBodySize   - Body 长度 (0 表示自动计算字符串长度)
int XTP_Send(xtcpserver* pServer, int iClientId, str sCmd, size_t iCmdSize, 
			 uint iParamCount, str* arrParam, str* arrValue, 
			 ptr pBody, size_t iBodySize)
{
	if ( pServer == NULL || sCmd == NULL ) return FALSE;
	
	// 自动计算长度
	if ( iCmdSize == 0 ) {
		iCmdSize = strlen(sCmd);
	}
	if ( pBody && (iBodySize == 0) ) {
		iBodySize = strlen(pBody);
	}
	
	// 检查参数数量上限（防止整数溢出）
	if ( iParamCount > 65535 ) {
		printf("XTP_Send: Too many parameters\n");
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
				printf("XTP_Send: Parameter too long\n");
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
		printf("XTP_Send: Packet too large\n");
		if ( pParamInfo ) xrtFree(pParamInfo);
		return FALSE;
	}
	
	// 分配发送缓冲区
	char* pSendBuf = xrtMalloc(iPackSize);
	if ( pSendBuf == NULL ) {
		if ( pParamInfo ) xrtFree(pParamInfo);
		return FALSE;
	}
	
	// 填充包头
	XTP_PackHeader* pHeader = (XTP_PackHeader*)pSendBuf;
	pHeader->HeadInfo[0] = 'x';
	pHeader->HeadInfo[1] = 't';
	pHeader->HeadInfo[2] = 'p';
	pHeader->HeadInfo[3] = 1;
	pHeader->PackSize = (unsigned int)iPackSize;
	pHeader->CmdSize = (unsigned short)iCmdSize;
	pHeader->ParamCount = (unsigned short)iParamCount;
	pHeader->BodySize = (unsigned int)iBodySize;
	
	// 填充参数信息头
	size_t iOffset = 16;
	if ( iParamCount > 0 && pParamInfo ) {
		memcpy(pSendBuf + iOffset, pParamInfo, iParamCount * sizeof(XTP_ParamInfo));
		iOffset += iParamCount * sizeof(XTP_ParamInfo);
	}
	
	// 填充命令
	if ( iCmdSize > 0 ) {
		memcpy(pSendBuf + iOffset, sCmd, iCmdSize);
		iOffset += iCmdSize;
	}
	
	// 填充参数键值对
	for ( uint i = 0; i < iParamCount; i++ ) {
		if ( pParamInfo[i].KeySize > 0 ) {
			memcpy(pSendBuf + iOffset, arrParam[i], pParamInfo[i].KeySize);
			iOffset += pParamInfo[i].KeySize;
		}
		if ( pParamInfo[i].ValSize > 0 ) {
			memcpy(pSendBuf + iOffset, arrValue[i], pParamInfo[i].ValSize);
			iOffset += pParamInfo[i].ValSize;
		}
	}
	
	// 填充 Body
	if ( iBodySize > 0 && pBody ) {
		memcpy(pSendBuf + iOffset, pBody, iBodySize);
	}
	
	// 发送数据
	xnet_result iRes = xrtTcpServerSend(pServer, iClientId, pSendBuf, iPackSize);
	
	// 释放内存
	xrtFree(pSendBuf);
	if ( pParamInfo ) {
		xrtFree(pParamInfo);
	}
	
	return (iRes == XRT_NET_OK) ? TRUE : FALSE;
}



// 获取 XTP 消息的参数值
char* XTP_GetParam(void* pMsg, const char* sKey)
{
	XTP_MessageObject msg = (XTP_MessageObject)pMsg;
	if ( msg == NULL || msg->Params == NULL || sKey == NULL ) return NULL;
	return xrtDictGetPtr(msg->Params, (ptr)sKey, strlen(sKey));
}



// ==================== XTP 协议处理回调 ====================

// XTP 连接接受回调
static void OnXtpAccept(ptr pOwner, xnetconn* pConn)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	// 初始化连接上下文
	XTP_InitContext(pConn->iId);
	
	// 调用脚本回调
	if ( objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_ACCEPT, NULL, 0);
	}
}

// XTP 数据接收回调
static void OnXtpRecv(ptr pOwner, xnetconn* pConn, const char* pData, size_t iLen)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	// 获取连接上下文
	XTP_ConnContext* pCtx = XTP_GetContext(pConn->iId);
	if ( pCtx == NULL || !pCtx->bInited ) {
		return;
	}
	
	// 追加数据到接收缓冲区
	xrtNetBufAppend(&pCtx->tRecvBuf, pData, iLen);
	
	// 处理所有完整的数据包
	while ( XTP_ProcessPacket(objServer, pConn, pCtx) > 0 ) {
		// 继续处理下一个包
	}
}

// XTP 连接关闭回调
static void OnXtpClose(ptr pOwner, xnetconn* pConn)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	// 调用脚本回调
	if ( objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_CLOSE, NULL, 0);
	}
	
	// 清理连接上下文
	XTP_CleanupContext(pConn->iId);
}



// ==================== XTP 服务启动/停止 ====================

// 启动 XTP 服务
int RunServerXTP(XS_ServerObject objServer)
{
	char sIP[64] = {0};
	uint16 iPort = 0;
	
	// 解析地址端口
	ParseAddr(objServer->Addr, sIP, sizeof(sIP), &iPort);
	
	// 配置 TCP 服务器
	xnetconfig tConfig = {0};
	tConfig.iRecvBufSize = 8192;
	tConfig.iMaxClients = 1024;
	
	// 分配连接上下文表
	g_iXtpMaxClients = tConfig.iMaxClients;
	g_arrXtpCtx = xrtMalloc(g_iXtpMaxClients * sizeof(XTP_ConnContext));
	if ( g_arrXtpCtx ) {
		memset(g_arrXtpCtx, 0, g_iXtpMaxClients * sizeof(XTP_ConnContext));
	}
	
	// 事件回调
	xnetevents tEvents = {0};
	tEvents.OnAccept = OnXtpAccept;
	tEvents.OnRecv = OnXtpRecv;
	tEvents.OnClose = OnXtpClose;
	
	// 创建 TCP 服务器 (共享事件循环)
	printf("    Run Server [XTP] : %s (%s:%d)\n", objServer->Name, sIP, iPort);
	xtcpserver* pServer = xrtTcpServerCreateEx(g_pLoop, sIP, iPort, &tConfig, &tEvents);
	if ( pServer == NULL ) {
		printf("    !!! ERROR !!! Cannot create XTP server on %s:%d\n", sIP, iPort);
		exit(EXIT_FAILURE);
	}
	
	// 设置用户数据为 Server 对象
	xrtTcpServerSetUserData(pServer, objServer);
	objServer->pServer = pServer;
	
	// 启动服务器
	if ( xrtTcpServerStart(pServer) != XRT_NET_OK ) {
		printf("    !!! ERROR !!! Cannot start XTP server\n");
		exit(EXIT_FAILURE);
	}
	
	// XTPS (XTP over TLS)
	if ( objServer->EnableTLS ) {
		uint16 iTlsPort = 0;
		ParseAddr(objServer->AddrTLS, sIP, sizeof(sIP), &iTlsPort);
		
		printf("    Run Server [XTPS] : %s (%s:%d)\n", objServer->Name, sIP, iTlsPort);
		
		// 创建 TLS XTP 服务器
		xtcpserver* pServerTLS = xrtTcpServerCreateEx(g_pLoop, sIP, iTlsPort, &tConfig, &tEvents);
		if ( pServerTLS == NULL ) {
			printf("    !!! ERROR !!! Cannot create XTPS server on %s:%d\n", sIP, iTlsPort);
			exit(EXIT_FAILURE);
		}
		
		xrtTcpServerSetUserData(pServerTLS, objServer);
		objServer->pServerTLS = pServerTLS;
		
		// 配置 TLS
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			if ( xrtTcpServerEnableTLS(pServerTLS, &objServer->DefaultHost.tTlsConfig) != XRT_NET_OK ) {
				printf("    !!! ERROR !!! Cannot enable TLS for XTPS server\n");
				exit(EXIT_FAILURE);
			}
		}
		
		// 启动 TLS XTP 服务器
		if ( xrtTcpServerStart(pServerTLS) != XRT_NET_OK ) {
			printf("    !!! ERROR !!! Cannot start XTPS server\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return TRUE;
}



// 停止 XTP 服务
int StopServerXTP(XS_ServerObject objServer)
{
	printf("    Stop Server [XTP] : %s\n", objServer->Name);
	
	if ( objServer->pServer ) {
		xrtTcpServerDestroy((xtcpserver*)objServer->pServer);
		objServer->pServer = NULL;
	}
	if ( objServer->pServerTLS ) {
		xrtTcpServerDestroy((xtcpserver*)objServer->pServerTLS);
		objServer->pServerTLS = NULL;
	}
	
	// 释放连接上下文表
	if ( g_arrXtpCtx ) {
		for ( int i = 0; i < g_iXtpMaxClients; i++ ) {
			if ( g_arrXtpCtx[i].bInited ) {
				xrtNetBufFree(&g_arrXtpCtx[i].tRecvBuf);
			}
		}
		xrtFree(g_arrXtpCtx);
		g_arrXtpCtx = NULL;
	}
	
	return TRUE;
}


