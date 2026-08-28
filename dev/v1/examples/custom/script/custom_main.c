#include <xsbase.h>
#include <stdio.h>
#include <string.h>

#include "app_db.h"



static volatile bool G_bStopScheduler = FALSE;
static xthread G_hScheduler = NULL;



typedef struct CUSTOM_TaskItem
{
	int64 iTaskID;
	char* sTaskName;
	char* sCommand;
	char* sWorkDir;
} CUSTOM_TaskItem;



static char* procBuildShellCommand(const char* sCommand, const char* sWorkDir)
{
	const char* sSafeCommand = sCommand ? sCommand : "";

	if ( sWorkDir && sWorkDir[0] ) {
#ifdef _WIN32
		return xrtFormat("cd /d \"%s\" && %s 2>&1", sWorkDir, sSafeCommand);
#else
		return xrtFormat("cd \"%s\" && %s 2>&1", sWorkDir, sSafeCommand);
#endif
	}

	return xrtFormat("%s 2>&1", sSafeCommand);
}



static bool procInsertRun(int64 iTaskID, const char* sTaskName, const char* sStatus, int iExitCode, const char* sOutput)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	char* sNow;
	int iRet;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return FALSE;
	}

	sNow = xrtNowStr();
	if ( sqlite3_prepare_v2(
		pDB,
		"INSERT INTO task_run (task_id, task_name, status, exit_code, output, created_at) VALUES (?, ?, ?, ?, ?, ?);",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		if ( sNow ) {
			xrtFree(sNow);
		}
		return FALSE;
	}

	sqlite3_bind_int64(pStmt, 1, iTaskID);
	sqlite3_bind_text(pStmt, 2, sTaskName ? sTaskName : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 3, sStatus ? sStatus : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(pStmt, 4, iExitCode);
	sqlite3_bind_text(pStmt, 5, sOutput ? sOutput : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 6, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static bool procMarkTaskDone(int64 iTaskID, const char* sStatus)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	char* sNow;
	int iRet;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return FALSE;
	}

	sNow = xrtNowStr();
	if ( sqlite3_prepare_v2(
		pDB,
		"UPDATE task SET enabled = 0, last_run_at = ?, last_status = ? WHERE id = ?;",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		if ( sNow ) {
			xrtFree(sNow);
		}
		return FALSE;
	}

	sqlite3_bind_text(pStmt, 1, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 2, sStatus ? sStatus : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(pStmt, 3, iTaskID);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static void procRunOneTask(int64 iTaskID, const char* sTaskName, const char* sCommand, const char* sWorkDir)
{
	char* sShellCommand;
	FILE* pPipe;
	char arrBuf[1024];
	char* sOutput;
	const char* sStatus;
	int iExitCode;
	size_t iTotal = 0;
	size_t iRead;

	sShellCommand = procBuildShellCommand(sCommand, sWorkDir);
	if ( sShellCommand == NULL ) {
		(void)procInsertRun(iTaskID, sTaskName, "failed", -1, "build shell command failed");
		(void)procMarkTaskDone(iTaskID, "failed");
		return;
	}

	pPipe = popen(sShellCommand, "r");
	if ( pPipe == NULL ) {
		xrtFree(sShellCommand);
		(void)procInsertRun(iTaskID, sTaskName, "failed", -1, "open process failed");
		(void)procMarkTaskDone(iTaskID, "failed");
		return;
	}

	sOutput = xrtMalloc(16385);
	if ( sOutput ) {
		sOutput[0] = '\0';
		while ( fgets(arrBuf, sizeof(arrBuf), pPipe) ) {
			iRead = strlen(arrBuf);
			if ( (iTotal + iRead) > 16384 ) {
				iRead = 16384 - iTotal;
			}
			if ( iRead > 0 ) {
				memcpy(sOutput + iTotal, arrBuf, iRead);
				iTotal += iRead;
				sOutput[iTotal] = '\0';
			}
			if ( iTotal >= 16384 ) {
				break;
			}
		}
	}
	if ( sOutput == NULL ) {
		sOutput = xrtCopyStr((str)"", 0);
	}

	iExitCode = pclose(pPipe);
	xrtFree(sShellCommand);
	sStatus = (iExitCode == 0) ? "success" : "failed";
	(void)procInsertRun(iTaskID, sTaskName, sStatus, iExitCode, sOutput ? sOutput : "");
	(void)procMarkTaskDone(iTaskID, sStatus);
	if ( sOutput ) {
		xrtFree(sOutput);
	}
}

static void procRunDueTasks(void)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	char* sNow;
	CUSTOM_TaskItem arrTask[16];
	int iTaskCount = 0;
	int iIndex;

	memset(arrTask, 0, sizeof(arrTask));

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return;
	}

	sNow = xrtNowStr();
	if ( sqlite3_prepare_v2(
		pDB,
		"SELECT id, name, command, workdir FROM task WHERE enabled = 1 AND run_at <= ? ORDER BY run_at ASC LIMIT 16;",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		if ( sNow ) {
			xrtFree(sNow);
		}
		return;
	}

	sqlite3_bind_text(pStmt, 1, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	while ( (iTaskCount < 16) && (sqlite3_step(pStmt) == SQLITE_ROW) ) {
		arrTask[iTaskCount].iTaskID = sqlite3_column_int64(pStmt, 0);
		arrTask[iTaskCount].sTaskName = xrtCopyStr((str)((const char*)sqlite3_column_text(pStmt, 1) ? (const char*)sqlite3_column_text(pStmt, 1) : ""), 0);
		arrTask[iTaskCount].sCommand = xrtCopyStr((str)((const char*)sqlite3_column_text(pStmt, 2) ? (const char*)sqlite3_column_text(pStmt, 2) : ""), 0);
		arrTask[iTaskCount].sWorkDir = xrtCopyStr((str)((const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : ""), 0);
		iTaskCount++;
	}

	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	for ( iIndex = 0; iIndex < iTaskCount; iIndex++ ) {
		procRunOneTask(
			arrTask[iIndex].iTaskID,
			arrTask[iIndex].sTaskName ? arrTask[iIndex].sTaskName : "",
			arrTask[iIndex].sCommand ? arrTask[iIndex].sCommand : "",
			arrTask[iIndex].sWorkDir ? arrTask[iIndex].sWorkDir : ""
		);
		if ( arrTask[iIndex].sTaskName ) {
			xrtFree(arrTask[iIndex].sTaskName);
		}
		if ( arrTask[iIndex].sCommand ) {
			xrtFree(arrTask[iIndex].sCommand);
		}
		if ( arrTask[iIndex].sWorkDir ) {
			xrtFree(arrTask[iIndex].sWorkDir);
		}
	}
}

static uint32 procSchedulerThread(ptr pParam)
{
	(void)pParam;

	while ( !G_bStopScheduler ) {
		procRunDueTasks();
		xrtSleep(1000);
	}

	return 0;
}



void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	sqlite3* pDB;
	const char* sAppPath;

	(void)objServer;
	(void)objHost;

	sAppPath = xsAppPath();
	if ( sAppPath && sAppPath[0] != '\0' ) {
		procDBSetAppPath(sAppPath);
	}

	pDB = procDBOpen();
	if ( pDB ) {
		sqlite3_close(pDB);
	}

	G_bStopScheduler = FALSE;
	if ( G_hScheduler == NULL ) {
		G_hScheduler = xrtThreadCreate((ptr)procSchedulerThread, NULL, 0);
	}
}



void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	(void)objServer;
	(void)objHost;

	G_bStopScheduler = TRUE;
	if ( G_hScheduler ) {
		xrtThreadWait(G_hScheduler);
		xrtThreadDestroy(G_hScheduler);
		G_hScheduler = NULL;
	}
}



void EventOpenProc(XS_ServerObject objServer, void* pStream)
{
	const char* sHello = "custom scheduler ready\n";

	(void)objServer;
	(void)pStream;
	(void)xsStreamSend(pStream, sHello, strlen(sHello));
}



bool EventDataProc(XS_ServerObject objServer, void* pStream, const void* pData, size_t iLen)
{
	char sReply[256];

	(void)objServer;
	(void)pData;

	snprintf(sReply, sizeof(sReply), "custom scheduler echo bytes=%u\n", (unsigned)iLen);
	return xsStreamSend(pStream, sReply, strlen(sReply)) != 0;
}
