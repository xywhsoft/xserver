


// 导入 xrt 网络 API 到 TCC
void ImportXrtNet(TCCState* s)
{
	// ==================== TCP 服务器 ====================
	tcc_add_symbol(s, "xrtTcpServerCreate", xrtTcpServerCreate);
	tcc_add_symbol(s, "xrtTcpServerCreateEx", xrtTcpServerCreateEx);
	tcc_add_symbol(s, "xrtTcpServerDestroy", xrtTcpServerDestroy);
	tcc_add_symbol(s, "xrtTcpServerStart", xrtTcpServerStart);
	tcc_add_symbol(s, "xrtTcpServerStop", xrtTcpServerStop);
	tcc_add_symbol(s, "xrtTcpServerSend", xrtTcpServerSend);
	tcc_add_symbol(s, "xrtTcpServerDisconnect", xrtTcpServerDisconnect);
	tcc_add_symbol(s, "xrtTcpServerEnableTLS", xrtTcpServerEnableTLS);
	tcc_add_symbol(s, "xrtTcpServerGetClientCount", xrtTcpServerGetClientCount);
	tcc_add_symbol(s, "xrtTcpServerSetUserData", xrtTcpServerSetUserData);
	tcc_add_symbol(s, "xrtTcpServerGetUserData", xrtTcpServerGetUserData);
	
	// ==================== TCP 客户端 ====================
	tcc_add_symbol(s, "xrtTcpClientCreate", xrtTcpClientCreate);
	tcc_add_symbol(s, "xrtTcpClientCreateEx", xrtTcpClientCreateEx);
	tcc_add_symbol(s, "xrtTcpClientDestroy", xrtTcpClientDestroy);
	tcc_add_symbol(s, "xrtTcpClientConnect", xrtTcpClientConnect);
	
	// ==================== UDP 服务器 ====================
	tcc_add_symbol(s, "xrtUdpServerCreate", xrtUdpServerCreate);
	tcc_add_symbol(s, "xrtUdpServerCreateEx", xrtUdpServerCreateEx);
	tcc_add_symbol(s, "xrtUdpServerDestroy", xrtUdpServerDestroy);
	tcc_add_symbol(s, "xrtUdpServerStart", xrtUdpServerStart);
	tcc_add_symbol(s, "xrtUdpServerStop", xrtUdpServerStop);
	tcc_add_symbol(s, "xrtUdpServerSendTo", xrtUdpServerSendTo);
	tcc_add_symbol(s, "xrtUdpServerSetUserData", xrtUdpServerSetUserData);
	tcc_add_symbol(s, "xrtUdpServerGetUserData", xrtUdpServerGetUserData);
	
	// ==================== HTTP 服务器 ====================
	tcc_add_symbol(s, "xrtHttpServerCreate", xrtHttpServerCreate);
	tcc_add_symbol(s, "xrtHttpServerCreateEx", xrtHttpServerCreateEx);
	tcc_add_symbol(s, "xrtHttpServerDestroy", xrtHttpServerDestroy);
	tcc_add_symbol(s, "xrtHttpServerStart", xrtHttpServerStart);
	tcc_add_symbol(s, "xrtHttpServerStop", xrtHttpServerStop);
	tcc_add_symbol(s, "xrtHttpServerEnableTLS", xrtHttpServerEnableTLS);
	tcc_add_symbol(s, "xrtHttpServerSetUserData", xrtHttpServerSetUserData);
	tcc_add_symbol(s, "xrtHttpServerGetUserData", xrtHttpServerGetUserData);
	
	// ==================== HTTP 响应 ====================
	tcc_add_symbol(s, "xrtHttpReply", xrtHttpReply);
	tcc_add_symbol(s, "xrtHttpReplyFmt", xrtHttpReplyFmt);
	tcc_add_symbol(s, "xrtHttpReplyJSON", xrtHttpReplyJSON);
	tcc_add_symbol(s, "xrtHttpReplyFile", xrtHttpReplyFile);
	tcc_add_symbol(s, "xrtHttpReplyStart", xrtHttpReplyStart);
	tcc_add_symbol(s, "xrtHttpReplyChunk", xrtHttpReplyChunk);
	tcc_add_symbol(s, "xrtHttpReplyEnd", xrtHttpReplyEnd);
	tcc_add_symbol(s, "xrtHttpRedirect", xrtHttpRedirect);
	
	// ==================== HTTP 请求 ====================
	tcc_add_symbol(s, "xrtHttpReqMatch", xrtHttpReqMatch);
	tcc_add_symbol(s, "xrtHttpReqGetHeader", xrtHttpReqGetHeader);
	tcc_add_symbol(s, "xrtHttpReqGetParam", xrtHttpReqGetParam);
	tcc_add_symbol(s, "xrtHttpReqGetCookie", xrtHttpReqGetCookie);
	
	// ==================== HTTP 静态文件 ====================
	tcc_add_symbol(s, "xrtHttpServeDir", xrtHttpServeDir);
	tcc_add_symbol(s, "xrtHttpServeFile", xrtHttpServeFile);
	tcc_add_symbol(s, "xrtHttpUpgradeWebSocket", xrtHttpUpgradeWebSocket);
	
	// ==================== WebSocket 服务器 ====================
	tcc_add_symbol(s, "xrtWsServerCreate", xrtWsServerCreate);
	tcc_add_symbol(s, "xrtWsServerCreateEx", xrtWsServerCreateEx);
	tcc_add_symbol(s, "xrtWsServerDestroy", xrtWsServerDestroy);
	tcc_add_symbol(s, "xrtWsServerStart", xrtWsServerStart);
	tcc_add_symbol(s, "xrtWsServerStop", xrtWsServerStop);
	tcc_add_symbol(s, "xrtWsServerEnableTLS", xrtWsServerEnableTLS);
	tcc_add_symbol(s, "xrtWsServerSendText", xrtWsServerSendText);
	tcc_add_symbol(s, "xrtWsServerSendBinary", xrtWsServerSendBinary);
	tcc_add_symbol(s, "xrtWsServerPing", xrtWsServerPing);
	tcc_add_symbol(s, "xrtWsServerBroadcastText", xrtWsServerBroadcastText);
	tcc_add_symbol(s, "xrtWsServerBroadcastBinary", xrtWsServerBroadcastBinary);
	tcc_add_symbol(s, "xrtWsServerDisconnect", xrtWsServerDisconnect);
	tcc_add_symbol(s, "xrtWsServerGetClientCount", xrtWsServerGetClientCount);
	tcc_add_symbol(s, "xrtWsServerSetUserData", xrtWsServerSetUserData);
	tcc_add_symbol(s, "xrtWsServerGetUserData", xrtWsServerGetUserData);
	
	// ==================== WebSocket 客户端 ====================
	tcc_add_symbol(s, "xrtWsClientCreate", xrtWsClientCreate);
	tcc_add_symbol(s, "xrtWsClientCreateEx", xrtWsClientCreateEx);
	tcc_add_symbol(s, "xrtWsClientDestroy", xrtWsClientDestroy);
	tcc_add_symbol(s, "xrtWsClientConnect", xrtWsClientConnect);
	tcc_add_symbol(s, "xrtWsClientDisconnect", xrtWsClientDisconnect);
	tcc_add_symbol(s, "xrtWsClientSendText", xrtWsClientSendText);
	tcc_add_symbol(s, "xrtWsClientSendBinary", xrtWsClientSendBinary);
	tcc_add_symbol(s, "xrtWsClientPing", xrtWsClientPing);
	tcc_add_symbol(s, "xrtWsClientClose", xrtWsClientClose);
	tcc_add_symbol(s, "xrtWsClientIsConnected", xrtWsClientIsConnected);
	tcc_add_symbol(s, "xrtWsClientSetUserData", xrtWsClientSetUserData);
	tcc_add_symbol(s, "xrtWsClientGetUserData", xrtWsClientGetUserData);
	
	// ==================== HTTP 客户端 ====================
	tcc_add_symbol(s, "xrtHttpGet", xrtHttpGet);
	tcc_add_symbol(s, "xrtHttpPost", xrtHttpPost);
	tcc_add_symbol(s, "xrtHttpGetFile", xrtHttpGetFile);
	tcc_add_symbol(s, "xrtHttpRespFree", xrtHttpRespFree);
	tcc_add_symbol(s, "xrtHttpReqCreate", xrtHttpReqCreate);
	tcc_add_symbol(s, "xrtHttpReqFree", xrtHttpReqFree);
	tcc_add_symbol(s, "xrtHttpReqSetHeader", xrtHttpReqSetHeader);
	tcc_add_symbol(s, "xrtHttpReqSetBody", xrtHttpReqSetBody);
	
	// ==================== TLS ====================
	tcc_add_symbol(s, "xrtTlsCreate", xrtTlsCreate);
	tcc_add_symbol(s, "xrtTlsDestroy", xrtTlsDestroy);
	tcc_add_symbol(s, "xrtTlsHandshake", xrtTlsHandshake);
	tcc_add_symbol(s, "xrtTlsRead", xrtTlsRead);
	tcc_add_symbol(s, "xrtTlsWrite", xrtTlsWrite);
	tcc_add_symbol(s, "xrtTlsClose", xrtTlsClose);
	tcc_add_symbol(s, "xrtTlsIsReady", xrtTlsIsReady);
	tcc_add_symbol(s, "xrtTlsGetSNI", xrtTlsGetSNI);
	tcc_add_symbol(s, "xrtTlsSetCert", xrtTlsSetCert);
	
	// ==================== 事件循环 ====================
	tcc_add_symbol(s, "xrtEventLoopCreate", xrtEventLoopCreate);
	tcc_add_symbol(s, "xrtEventLoopDestroy", xrtEventLoopDestroy);
	tcc_add_symbol(s, "xrtEventLoopRun", xrtEventLoopRun);
	tcc_add_symbol(s, "xrtEventLoopStop", xrtEventLoopStop);
	tcc_add_symbol(s, "xrtEventLoopRunOnce", xrtEventLoopRunOnce);
	
	// ==================== 缓冲区 ====================
	tcc_add_symbol(s, "xrtNetBufInit", xrtNetBufInit);
	tcc_add_symbol(s, "xrtNetBufFree", xrtNetBufFree);
	tcc_add_symbol(s, "xrtNetBufAppend", xrtNetBufAppend);
	tcc_add_symbol(s, "xrtNetBufConsume", xrtNetBufConsume);
	tcc_add_symbol(s, "xrtNetBufClear", xrtNetBufClear);
	
	// ==================== 地址 ====================
	tcc_add_symbol(s, "xrtNetAddrInit", xrtNetAddrInit);
}


