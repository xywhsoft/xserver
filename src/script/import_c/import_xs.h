#ifndef XS_SCRIPT_IMPORT_C_IMPORT_XS_H
#define XS_SCRIPT_IMPORT_C_IMPORT_XS_H



static inline void XS_ImportXSSymbols(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	/* runtime */
	tcc_add_symbol(s, "xsLog", XS_ScriptLog);
	tcc_add_symbol(s, "xsServerName", XS_ScriptServerName);
	tcc_add_symbol(s, "xsServerClass", XS_ScriptServerClass);
	tcc_add_symbol(s, "xsServerDebug", XS_ScriptServerDebug);
	tcc_add_symbol(s, "xsServerAddr", XS_ScriptServerAddr);
	tcc_add_symbol(s, "xsServerParam", XS_ScriptServerParam);
	tcc_add_symbol(s, "xsAppPath", XS_ScriptAppPath);
	tcc_add_symbol(s, "xsHostName", XS_ScriptHostName);
	tcc_add_symbol(s, "xsHostParam", XS_ScriptHostParam);
	tcc_add_symbol(s, "xsHostPath", XS_ScriptHostPath);
	tcc_add_symbol(s, "xsHostDevFile", XS_ScriptHostDevFile);
	tcc_add_symbol(s, "xsHostDebug", XS_ScriptHostDebug);
	tcc_add_symbol(s, "xsHostDevMode", XS_ScriptHostDevMode);
	tcc_add_symbol(s, "xsReloadCurrentHost", XS_ScriptReloadCurrentHost);
	tcc_add_symbol(s, "xsReloadHostByName", XS_ScriptReloadHostByName);

	/* request */
	tcc_add_symbol(s, "xsReqMethod", XS_ScriptRequestMethod);
	tcc_add_symbol(s, "xsReqTarget", XS_ScriptRequestTarget);
	tcc_add_symbol(s, "xsReqPath", XS_ScriptRequestPath);
	tcc_add_symbol(s, "xsReqQuery", XS_ScriptRequestQuery);
	tcc_add_symbol(s, "xsReqBody", XS_ScriptRequestBody);
	tcc_add_symbol(s, "xsReqBodyLen", XS_ScriptRequestBodyLen);
	tcc_add_symbol(s, "xsReqRemote", XS_ScriptRequestRemote);
	tcc_add_symbol(s, "xsReqHeader", XS_ScriptRequestHeader);

	/* response */
	tcc_add_symbol(s, "xsHttpStatus", XS_ScriptHttpStatus);
	tcc_add_symbol(s, "xsHttpHeader", XS_ScriptHttpHeader);
	tcc_add_symbol(s, "xsHttpText", XS_ScriptHttpText);
	tcc_add_symbol(s, "xsHttpBody", XS_ScriptHttpBody);
	tcc_add_symbol(s, "xsHttpJson", XS_ScriptHttpJson);
	tcc_add_symbol(s, "xsRet403", XS_ScriptRet403);
	tcc_add_symbol(s, "xsRet404", XS_ScriptRet404);
	tcc_add_symbol(s, "xsRet500", XS_ScriptRet500);
	tcc_add_symbol(s, "xsRetError", XS_ScriptRetError);
	tcc_add_symbol(s, "Ret403", XS_ScriptRet403);
	tcc_add_symbol(s, "Ret404", XS_ScriptRet404);
	tcc_add_symbol(s, "Ret500", XS_ScriptRet500);
	tcc_add_symbol(s, "RetError", XS_ScriptRetError);

	/* websocket */
	tcc_add_symbol(s, "xsWsIsOpen", XS_ScriptWsIsOpen);
	tcc_add_symbol(s, "xsWsProtocol", XS_ScriptWsProtocol);
	tcc_add_symbol(s, "xsWsClose", XS_ScriptWsClose);
	tcc_add_symbol(s, "xsWsSendText", XS_ScriptWsSendText);
	tcc_add_symbol(s, "xsWsSendBinary", XS_ScriptWsSendBinary);
	tcc_add_symbol(s, "xsWsPing", XS_ScriptWsPing);

	/* stream */
	tcc_add_symbol(s, "xsStreamSend", XS_ScriptStreamSend);
	tcc_add_symbol(s, "xsStreamClose", XS_ScriptStreamClose);

	/* dgram */
	tcc_add_symbol(s, "xsDgramSendTo", XS_ScriptDgramSendTo);
	tcc_add_symbol(s, "xsDgramReply", XS_ScriptDgramReply);
	tcc_add_symbol(s, "xsAddrText", XS_ScriptAddrText);

	/* xtp message */
	tcc_add_symbol(s, "xsXtpSend", XS_ScriptXtpSend);
	tcc_add_symbol(s, "xsXtpSendRequest", XS_ScriptXtpSendRequest);
	tcc_add_symbol(s, "xsXtpSendPush", XS_ScriptXtpSendPush);
	tcc_add_symbol(s, "xsXtpSendEvent", XS_ScriptXtpSendEvent);
	tcc_add_symbol(s, "xsXtpSendEx", XS_ScriptXtpSendEx);
	tcc_add_symbol(s, "xsXtpGetParam", XS_ScriptXtpGetParam);
	tcc_add_symbol(s, "xsXtpReply", XS_ScriptXtpReply);
	tcc_add_symbol(s, "xsXtpReplyEx", XS_ScriptXtpReplyEx);
	tcc_add_symbol(s, "xsXtpMsgId", XS_ScriptXtpMsgId);
	tcc_add_symbol(s, "xsXtpMsgType", XS_ScriptXtpMsgType);
	tcc_add_symbol(s, "xsXtpMsgFlags", XS_ScriptXtpMsgFlags);
	tcc_add_symbol(s, "xsXtpIsOK", XS_ScriptXtpIsOK);
	tcc_add_symbol(s, "xsXtpStatus", XS_ScriptXtpStatus);
	tcc_add_symbol(s, "xsXtpCmd", XS_ScriptXtpCmd);
	tcc_add_symbol(s, "xsXtpCmdLen", XS_ScriptXtpCmdLen);
	tcc_add_symbol(s, "xsXtpBody", XS_ScriptXtpBody);
	tcc_add_symbol(s, "xsXtpBodyLen", XS_ScriptXtpBodyLen);
	tcc_add_symbol(s, "xsXtpBodyDup", XS_ScriptXtpBodyDup);
	tcc_add_symbol(s, "xsXtpBodyValue", XS_ScriptXtpBodyValue);
	tcc_add_symbol(s, "xsXtpCmdDup", XS_ScriptXtpCmdDup);
	tcc_add_symbol(s, "xsXtpParamsValue", XS_ScriptXtpParamsValue);
	tcc_add_symbol(s, "xsXtpValue", XS_ScriptXtpValue);
	tcc_add_symbol(s, "xsXtpMetaText", XS_ScriptXtpMetaText);
	tcc_add_symbol(s, "xsXtpMetaJson", XS_ScriptXtpMetaJson);
	tcc_add_symbol(s, "xsXtpResultJson", XS_ScriptXtpResultJson);
	tcc_add_symbol(s, "xsXtpErrorJson", XS_ScriptXtpErrorJson);
	tcc_add_symbol(s, "xsXtpSummaryText", XS_ScriptXtpSummaryText);
	tcc_add_symbol(s, "xsXtpSummaryJson", XS_ScriptXtpSummaryJson);
	tcc_add_symbol(s, "xsXtpParamCount", XS_ScriptXtpParamCount);
	tcc_add_symbol(s, "xsXtpNeedReply", XS_ScriptXtpNeedReply);
	tcc_add_symbol(s, "xsXtpIsRequest", XS_ScriptXtpIsRequest);
	tcc_add_symbol(s, "xsXtpIsResponse", XS_ScriptXtpIsResponse);
	tcc_add_symbol(s, "xsXtpIsPush", XS_ScriptXtpIsPush);
	tcc_add_symbol(s, "xsXtpIsEvent", XS_ScriptXtpIsEvent);
	tcc_add_symbol(s, "xsXtpCmdIs", XS_ScriptXtpCmdIs);
	tcc_add_symbol(s, "xsXtpHasParam", XS_ScriptXtpHasParam);
	tcc_add_symbol(s, "xsXtpParamText", XS_ScriptXtpParamText);
	tcc_add_symbol(s, "xsXtpResultText", XS_ScriptXtpResultText);
	tcc_add_symbol(s, "xsXtpResultIs", XS_ScriptXtpResultIs);
	tcc_add_symbol(s, "xsXtpStatusIs", XS_ScriptXtpStatusIs);
	tcc_add_symbol(s, "xsXtpErrorText", XS_ScriptXtpErrorText);
	tcc_add_symbol(s, "xsXtpErrorValue", XS_ScriptXtpErrorValue);
	tcc_add_symbol(s, "xsXtpParamDup", XS_ScriptXtpParamDup);
	tcc_add_symbol(s, "xsXtpParamInt", XS_ScriptXtpParamInt);
	tcc_add_symbol(s, "xsXtpParamBool", XS_ScriptXtpParamBool);
	tcc_add_symbol(s, "xsXtpReplyText", XS_ScriptXtpReplyText);
	tcc_add_symbol(s, "xsXtpReplyJson", XS_ScriptXtpReplyJson);
	tcc_add_symbol(s, "xsXtpReplyOKText", XS_ScriptXtpReplyOKText);
	tcc_add_symbol(s, "xsXtpReplyErrorText", XS_ScriptXtpReplyErrorText);
	tcc_add_symbol(s, "xsXtpReplyOKJson", XS_ScriptXtpReplyOKJson);
	tcc_add_symbol(s, "xsXtpReplyErrorJson", XS_ScriptXtpReplyErrorJson);
	tcc_add_symbol(s, "xsXtpReplyMissingParam", XS_ScriptXtpReplyMissingParam);
	tcc_add_symbol(s, "xsXtpReplyUnsupportedCmd", XS_ScriptXtpReplyUnsupportedCmd);
	tcc_add_symbol(s, "xsXtpParamKeyAt", XS_ScriptXtpParamKeyAt);
	tcc_add_symbol(s, "xsXtpParamValueAt", XS_ScriptXtpParamValueAt);
	tcc_add_symbol(s, "xsXtpFindParamView", XS_ScriptXtpFindParamView);

	/* xtp client */
	tcc_add_symbol(s, "xsXtpClientOpen", XS_ScriptXtpClientOpen);
	tcc_add_symbol(s, "xsXtpClientClose", XS_ScriptXtpClientClose);
	tcc_add_symbol(s, "xsXtpClientDo", XS_ScriptXtpClientDo);
	tcc_add_symbol(s, "xsXtpClientDoText", XS_ScriptXtpClientDoText);
	tcc_add_symbol(s, "xsXtpClientDoSimple", XS_ScriptXtpClientDoSimple);
	tcc_add_symbol(s, "xsXtpClientDoJson", XS_ScriptXtpClientDoJson);
	tcc_add_symbol(s, "xsXtpClientCall", XS_ScriptXtpClientCall);
	tcc_add_symbol(s, "xsXtpClientCallText", XS_ScriptXtpClientCallText);
	tcc_add_symbol(s, "xsXtpClientCallSimple", XS_ScriptXtpClientCallSimple);
	tcc_add_symbol(s, "xsXtpClientCallJson", XS_ScriptXtpClientCallJson);
	tcc_add_symbol(s, "xsXtpClientCallSimpleBody", XS_ScriptXtpClientCallSimpleBody);
	tcc_add_symbol(s, "xsXtpClientCallTextBody", XS_ScriptXtpClientCallTextBody);
	tcc_add_symbol(s, "xsXtpClientCallJsonBody", XS_ScriptXtpClientCallJsonBody);
	tcc_add_symbol(s, "xsXtpClientCallSimpleSummary", XS_ScriptXtpClientCallSimpleSummary);
	tcc_add_symbol(s, "xsXtpClientCallSimpleSummaryJson", XS_ScriptXtpClientCallSimpleSummaryJson);
	tcc_add_symbol(s, "xsXtpClientCallSimpleResult", XS_ScriptXtpClientCallSimpleResult);
	tcc_add_symbol(s, "xsXtpClientCallSimpleError", XS_ScriptXtpClientCallSimpleError);
	tcc_add_symbol(s, "xsXtpClientCallSimpleMeta", XS_ScriptXtpClientCallSimpleMeta);
	tcc_add_symbol(s, "xsXtpClientCallSimpleMetaJson", XS_ScriptXtpClientCallSimpleMetaJson);
	tcc_add_symbol(s, "xsXtpClientCallSimpleResultJson", XS_ScriptXtpClientCallSimpleResultJson);
	tcc_add_symbol(s, "xsXtpClientCallSimpleErrorJson", XS_ScriptXtpClientCallSimpleErrorJson);
	tcc_add_symbol(s, "xsXtpClientCallSimpleStatus", XS_ScriptXtpClientCallSimpleStatus);
	tcc_add_symbol(s, "xsXtpClientCallSimpleCmd", XS_ScriptXtpClientCallSimpleCmd);
	tcc_add_symbol(s, "xsXtpClientCallSimpleValue", XS_ScriptXtpClientCallSimpleValue);
	tcc_add_symbol(s, "xsXtpClientCallSimpleParamsValue", XS_ScriptXtpClientCallSimpleParamsValue);
	tcc_add_symbol(s, "xsXtpClientCallSimpleBodyValue", XS_ScriptXtpClientCallSimpleBodyValue);
	tcc_add_symbol(s, "xsXtpRequestCreate", XS_ScriptXtpRequestCreate);
	tcc_add_symbol(s, "xsXtpRequestFree", XS_ScriptXtpRequestFree);
	tcc_add_symbol(s, "xsXtpRequestSetCmd", XS_ScriptXtpRequestSetCmd);
	tcc_add_symbol(s, "xsXtpRequestSetParamsValue", XS_ScriptXtpRequestSetParamsValue);
	tcc_add_symbol(s, "xsXtpRequestSetParamText", XS_ScriptXtpRequestSetParamText);
	tcc_add_symbol(s, "xsXtpRequestSetParamInt", XS_ScriptXtpRequestSetParamInt);
	tcc_add_symbol(s, "xsXtpRequestSetParamBool", XS_ScriptXtpRequestSetParamBool);
	tcc_add_symbol(s, "xsXtpRequestSetBodyText", XS_ScriptXtpRequestSetBodyText);
	tcc_add_symbol(s, "xsXtpRequestSetBodyJson", XS_ScriptXtpRequestSetBodyJson);
	tcc_add_symbol(s, "xsXtpRequestSetBodyValue", XS_ScriptXtpRequestSetBodyValue);
	tcc_add_symbol(s, "xsXtpClientDoRequest", XS_ScriptXtpClientDoRequest);
	tcc_add_symbol(s, "xsXtpClientCallRequest", XS_ScriptXtpClientCallRequest);
	tcc_add_symbol(s, "xsXtpClientCallRequestValue", XS_ScriptXtpClientCallRequestValue);
	tcc_add_symbol(s, "xsXtpClientCallRequestParamsValue", XS_ScriptXtpClientCallRequestParamsValue);
	tcc_add_symbol(s, "xsXtpClientCallRequestBodyValue", XS_ScriptXtpClientCallRequestBodyValue);
	tcc_add_symbol(s, "xsXtpClientCallRequestBody", XS_ScriptXtpClientCallRequestBody);
	tcc_add_symbol(s, "xsXtpClientCallRequestResult", XS_ScriptXtpClientCallRequestResult);
	tcc_add_symbol(s, "xsXtpClientCallRequestError", XS_ScriptXtpClientCallRequestError);
	tcc_add_symbol(s, "xsXtpClientCallRequestMeta", XS_ScriptXtpClientCallRequestMeta);
	tcc_add_symbol(s, "xsXtpClientCallRequestMetaJson", XS_ScriptXtpClientCallRequestMetaJson);
	tcc_add_symbol(s, "xsXtpClientCallRequestResultJson", XS_ScriptXtpClientCallRequestResultJson);
	tcc_add_symbol(s, "xsXtpClientCallRequestErrorJson", XS_ScriptXtpClientCallRequestErrorJson);
	tcc_add_symbol(s, "xsXtpClientCallRequestOK", XS_ScriptXtpClientCallRequestOK);
	tcc_add_symbol(s, "xsXtpClientCallRequestStatus", XS_ScriptXtpClientCallRequestStatus);
	tcc_add_symbol(s, "xsXtpClientCallRequestCmd", XS_ScriptXtpClientCallRequestCmd);
	tcc_add_symbol(s, "xsXtpClientCallRequestSummary", XS_ScriptXtpClientCallRequestSummary);
	tcc_add_symbol(s, "xsXtpClientCallRequestSummaryJson", XS_ScriptXtpClientCallRequestSummaryJson);
	tcc_add_symbol(s, "xsXtpBuildParamArrays", XS_ScriptXtpBuildParamArrays);
	tcc_add_symbol(s, "xsXtpFreeParamArrays", XS_ScriptXtpFreeParamArrays);
	tcc_add_symbol(s, "xsXtpClientCallTableText", XS_ScriptXtpClientCallTableText);
	tcc_add_symbol(s, "xsXtpClientCallTableJson", XS_ScriptXtpClientCallTableJson);
	tcc_add_symbol(s, "xsXtpClientCallTableValue", XS_ScriptXtpClientCallTableValue);
	tcc_add_symbol(s, "xsXtpMessageFree", XS_ScriptXtpMessageFree);
	tcc_add_symbol(s, "xsXtpClientLastErrorCode", XS_ScriptXtpClientLastErrorCode);
	tcc_add_symbol(s, "xsXtpClientLastError", XS_ScriptXtpClientLastError);

	/* bus data */
	tcc_add_symbol(s, "xsDataRegister", XS_ScriptDataRegister);
	tcc_add_symbol(s, "xsDataRegisterEx", XS_ScriptDataRegisterEx);
	tcc_add_symbol(s, "xsDataGet", XS_ScriptDataGet);
	tcc_add_symbol(s, "xsDataRetain", XS_ScriptDataRetain);
	tcc_add_symbol(s, "xsDataRelease", XS_ScriptDataRelease);
	tcc_add_symbol(s, "xsDataRemove", XS_ScriptDataRemove);
	tcc_add_symbol(s, "xsDataRemoveByQuery", XS_ScriptDataRemoveByQuery);
	tcc_add_symbol(s, "xsDataFindFirst", XS_ScriptDataFindFirst);

	/* bus message */
	tcc_add_symbol(s, "xsBusStatusJson", XS_ScriptBusStatusJson);
	tcc_add_symbol(s, "xsBusStatusJsonEx", XS_ScriptBusStatusJsonEx);
	tcc_add_symbol(s, "xsBusNamespaceJson", XS_ScriptBusNamespaceJson);
	tcc_add_symbol(s, "xsBusLastErrorCode", XS_ScriptBusLastErrorCode);
	tcc_add_symbol(s, "xsBusLastError", XS_ScriptBusLastError);
	tcc_add_symbol(s, "xsMsgSendToHost", XS_ScriptMsgSendToHost);
	tcc_add_symbol(s, "xsMsgSendToServer", XS_ScriptMsgSendToServer);
	tcc_add_symbol(s, "xsMsgBroadcast", XS_ScriptMsgBroadcast);

	tcc_add_symbol(s, "XS_ImportXSSymbols", XS_ImportXSSymbols);
}

#endif
