#include <xsbase.h>
#include <string.h>

#include "app_db.h"



static xvalue procBodyJSON(XS_RequestObject objReq)
{
	const void* pBody = xsReqBody(objReq);
	size_t iBodyLen = xsReqBodyLen(objReq);

	if ( pBody == NULL || iBodyLen == 0 ) {
		return NULL;
	}

	return xrtParseJSON((void*)pBody, iBodyLen);
}



bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath;
	const char* sAppPath;
	xvalue objBody;
	const char* sName;
	const char* sRunAt;
	const char* sCommand;
	const char* sWorkDir;
	int64 iTaskID;
	int iEnabled;

	(void)objServer;
	(void)objHost;

	sAppPath = xsAppPath();
	if ( sAppPath && sAppPath[0] != '\0' ) {
		procDBSetAppPath(sAppPath);
	}

	sPath = xsReqPath(objReq);
	if ( sPath == NULL ) {
		return FALSE;
	}
	if ( strcmp(sPath, "/api/task/list") == 0 ) {
		return procReplyJSONValue(objResp, procTaskListValue());
	}
	if ( strcmp(sPath, "/api/run/list") == 0 ) {
		return procReplyJSONValue(objResp, procRunListValue());
	}
	if ( strcmp(sPath, "/api/task/add") == 0 ) {
		bool bOK;

		objBody = procBodyJSON(objReq);
		if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
			if ( objBody ) {
				xvoUnref(objBody);
			}
			return procReplyResult(objResp, FALSE, "body must be json object");
		}
		sName = xvoTableGetText(objBody, "name", 4);
		sRunAt = xvoTableGetText(objBody, "run_at", 6);
		sCommand = xvoTableGetText(objBody, "command", 7);
		sWorkDir = xvoTableGetText(objBody, "workdir", 7);
		if ( sName == NULL || sName[0] == '\0' || sRunAt == NULL || sRunAt[0] == '\0' || sCommand == NULL || sCommand[0] == '\0' ) {
			xvoUnref(objBody);
			return procReplyResult(objResp, FALSE, "name run_at command required");
		}
		bOK = procAddTask(sName, sRunAt, sCommand, sWorkDir);
		xvoUnref(objBody);
		return procReplyResult(objResp, bOK, "task created");
	}
	if ( strcmp(sPath, "/api/task/del") == 0 ) {
		objBody = procBodyJSON(objReq);
		if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
			if ( objBody ) {
				xvoUnref(objBody);
			}
			return procReplyResult(objResp, FALSE, "body must be json object");
		}
		iTaskID = xvoTableGetInt(objBody, "id", 2);
		xvoUnref(objBody);
		return procReplyResult(objResp, procDeleteTask(iTaskID), "task deleted");
	}
	if ( strcmp(sPath, "/api/task/toggle") == 0 ) {
		objBody = procBodyJSON(objReq);
		if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
			if ( objBody ) {
				xvoUnref(objBody);
			}
			return procReplyResult(objResp, FALSE, "body must be json object");
		}
		iTaskID = xvoTableGetInt(objBody, "id", 2);
		iEnabled = (int)xvoTableGetInt(objBody, "enabled", 7);
		xvoUnref(objBody);
		return procReplyResult(objResp, procToggleTask(iTaskID, iEnabled), "task updated");
	}

	return FALSE;
}
