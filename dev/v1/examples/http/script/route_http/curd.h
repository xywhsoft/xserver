bool Request_List(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	sqlite3_stmt* pStmt = NULL;
	xvalue objRet = xvoCreateTable();
	xvalue arrList = xvoCreateArray();
	bool bRet;
	int iStep;
	int iCount = 0;

	(void)objServer;
	(void)objHost;
	(void)objReq;

	if ( !DemoSQLitePrepare(&pStmt, "SELECT id, name, age, mail, \"desc\" FROM test;") ) {
		xvoUnref(arrList);
		xvoUnref(objRet);
		return DemoHttpReplyDBError(objResp, "query failed");
	}

	while ( (iStep = sqlite3_step(pStmt)) == SQLITE_ROW ) {
		xvalue objRow = xvoCreateTable();
		const char* sID = (const char*)sqlite3_column_text(pStmt, 0);
		const char* sName = (const char*)sqlite3_column_text(pStmt, 1);
		const char* sAge = (const char*)sqlite3_column_text(pStmt, 2);
		const char* sMail = (const char*)sqlite3_column_text(pStmt, 3);
		const char* sDesc = (const char*)sqlite3_column_text(pStmt, 4);

		xvoTableSetText(objRow, "id", 2, sID ? sID : "", 0, FALSE);
		xvoTableSetText(objRow, "name", 4, sName ? sName : "", 0, FALSE);
		xvoTableSetText(objRow, "age", 3, sAge ? sAge : "", 0, FALSE);
		xvoTableSetText(objRow, "mail", 4, sMail ? sMail : "", 0, FALSE);
		xvoTableSetText(objRow, "desc", 4, sDesc ? sDesc : "", 0, FALSE);
		xvoArrayAppendValue(arrList, objRow, TRUE);
		iCount++;
	}

	if ( iStep != SQLITE_DONE ) {
		sqlite3_finalize(pStmt);
		xvoUnref(arrList);
		xvoUnref(objRet);
		return DemoHttpReplyDBError(objResp, "query failed");
	}

	sqlite3_finalize(pStmt);

	xvoTableSetInt(objRet, "count", 5, iCount);
	xvoTableSetInt(objRet, "code", 4, 0);
	xvoTableSetText(objRet, "msg", 3, "platform list query success", 0, FALSE);
	xvoTableSetValue(objRet, "data", 4, arrList, TRUE);

	bRet = DemoHttpReplyJSONValue(objResp, 200, "OK", objRet);
	xvoUnref(objRet);
	return bRet;
}



bool Request_Add(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	sqlite3_stmt* pStmt = NULL;
	xvalue objBody = NULL;
	const char* sName;
	const char* sMail;
	const char* sDesc;
	int iAge;
	int iResult;
	const void* pBody;
	size_t iBodyLen;

	(void)objServer;
	(void)objHost;

	pBody = xsReqBody(objReq);
	iBodyLen = xsReqBodyLen(objReq);
	if ( pBody == NULL || iBodyLen == 0 ) {
		return DemoHttpReplyResult(objResp, FALSE, "body required");
	}

	objBody = xrtParseJSON(pBody, iBodyLen);
	if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
		if ( objBody ) {
			xvoUnref(objBody);
		}
		return DemoHttpReplyResult(objResp, FALSE, "body must be json object");
	}

	sName = xvoTableGetText(objBody, "name", 4);
	if ( sName == NULL || sName[0] == '\0' ) {
		xvoUnref(objBody);
		return DemoHttpReplyResult(objResp, FALSE, "name required");
	}

	iAge = xvoTableGetInt(objBody, "age", 3);
	sMail = xvoTableGetText(objBody, "mail", 4);
	sDesc = xvoTableGetText(objBody, "desc", 4);

	if ( !DemoSQLitePrepare(&pStmt, "INSERT INTO test (name, age, mail, \"desc\") VALUES (?, ?, ?, ?);") ) {
		xvoUnref(objBody);
		return DemoHttpReplyDBError(objResp, "prepare insert failed");
	}

	DemoSQLiteBindTextOrEmpty(pStmt, 1, sName);
	sqlite3_bind_int64(pStmt, 2, iAge);
	DemoSQLiteBindTextOrEmpty(pStmt, 3, sMail);
	DemoSQLiteBindTextOrEmpty(pStmt, 4, sDesc);

	iResult = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	xvoUnref(objBody);
	if ( iResult != SQLITE_DONE ) {
		return DemoHttpReplyDBError(objResp, "insert failed");
	}

	return DemoHttpReplyResult(objResp, TRUE, "add success");
}



bool Request_Del(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	sqlite3_stmt* pStmt = NULL;
	xvalue objBody = NULL;
	int iCount;
	int i;
	const void* pBody;
	size_t iBodyLen;

	(void)objServer;
	(void)objHost;

	pBody = xsReqBody(objReq);
	iBodyLen = xsReqBodyLen(objReq);
	if ( pBody == NULL || iBodyLen == 0 ) {
		return DemoHttpReplyResult(objResp, FALSE, "body required");
	}

	objBody = xrtParseJSON(pBody, iBodyLen);
	if ( objBody == NULL || objBody->Type != XVO_DT_ARRAY ) {
		if ( objBody ) {
			xvoUnref(objBody);
		}
		return DemoHttpReplyResult(objResp, FALSE, "body must be json array");
	}

	if ( !DemoSQLitePrepare(&pStmt, "DELETE FROM test WHERE id = ?;") ) {
		xvoUnref(objBody);
		return DemoHttpReplyDBError(objResp, "prepare delete failed");
	}

	iCount = xvoArrayItemCount(objBody);
	for ( i = 0; i < iCount; i++ ) {
		const char* sID = xvoArrayGetText(objBody, i);
		char* sIDText = NULL;
		int64 iID = 0;

		if ( sID == NULL || sID[0] == '\0' ) {
			iID = xvoArrayGetInt(objBody, i);
			if ( iID > 0 ) {
				sIDText = xrtFormat("%lld", (long long)iID);
				sID = sIDText;
			}
		}
		if ( sID == NULL || sID[0] == '\0' ) {
			if ( sIDText ) {
				xrtFree(sIDText);
			}
			continue;
		}

		DemoSQLiteBindTextOrEmpty(pStmt, 1, sID);
		if ( sqlite3_step(pStmt) != SQLITE_DONE ) {
			sqlite3_finalize(pStmt);
			xvoUnref(objBody);
			if ( sIDText ) {
				xrtFree(sIDText);
			}
			return DemoHttpReplyDBError(objResp, "delete failed");
		}
		sqlite3_reset(pStmt);
		sqlite3_clear_bindings(pStmt);
		if ( sIDText ) {
			xrtFree(sIDText);
		}
	}

	sqlite3_finalize(pStmt);
	xvoUnref(objBody);
	return DemoHttpReplyResult(objResp, TRUE, "delete success");
}



bool Request_Edit(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	sqlite3_stmt* pStmt = NULL;
	xvalue objBody = NULL;
	const char* sID;
	const char* sField;
	const char* sValue;
	const char* sSQL = NULL;
	int iResult;
	const void* pBody;
	size_t iBodyLen;

	(void)objServer;
	(void)objHost;

	pBody = xsReqBody(objReq);
	iBodyLen = xsReqBodyLen(objReq);
	if ( pBody == NULL || iBodyLen == 0 ) {
		return DemoHttpReplyResult(objResp, FALSE, "body required");
	}

	objBody = xrtParseJSON(pBody, iBodyLen);
	if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
		if ( objBody ) {
			xvoUnref(objBody);
		}
		return DemoHttpReplyResult(objResp, FALSE, "body must be json object");
	}

	sID = xvoTableGetText(objBody, "id", 2);
	sField = xvoTableGetText(objBody, "field", 5);
	sValue = xvoTableGetText(objBody, "value", 5);

	if ( sID == NULL || sID[0] == '\0' ) {
		xvoUnref(objBody);
		return DemoHttpReplyResult(objResp, FALSE, "id required");
	}
	if ( sField == NULL || sField[0] == '\0' ) {
		xvoUnref(objBody);
		return DemoHttpReplyResult(objResp, FALSE, "field required");
	}

	if ( strcmp(sField, "name") == 0 ) {
		sSQL = "UPDATE test SET name = ? WHERE id = ?;";
	} else if ( strcmp(sField, "age") == 0 ) {
		sSQL = "UPDATE test SET age = ? WHERE id = ?;";
	} else if ( strcmp(sField, "mail") == 0 ) {
		sSQL = "UPDATE test SET mail = ? WHERE id = ?;";
	} else if ( strcmp(sField, "desc") == 0 ) {
		sSQL = "UPDATE test SET \"desc\" = ? WHERE id = ?;";
	} else {
		xvoUnref(objBody);
		return DemoHttpReplyResult(objResp, FALSE, "invalid field");
	}

	if ( !DemoSQLitePrepare(&pStmt, sSQL) ) {
		xvoUnref(objBody);
		return DemoHttpReplyDBError(objResp, "prepare update failed");
	}

	DemoSQLiteBindTextOrEmpty(pStmt, 1, sValue);
	DemoSQLiteBindTextOrEmpty(pStmt, 2, sID);
	iResult = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	xvoUnref(objBody);
	if ( iResult != SQLITE_DONE ) {
		return DemoHttpReplyDBError(objResp, "update failed");
	}

	return DemoHttpReplyResult(objResp, TRUE, "edit success");
}
