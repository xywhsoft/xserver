/* extracted runtime state api */

static inline void XS_LockCheckConfigState(void)
{
	while ( __xrtAtomicCompareExchange32(&g_iXsCheckConfigStateLock, 1, 0) != 0 ) {
	}
}

static inline void XS_UnlockCheckConfigState(void)
{
	(void)__xrtAtomicExchange32(&g_iXsCheckConfigStateLock, 0);
}

static inline void XS_GetCheckConfigStatusSnapshot(XS_CheckConfigStatusSnapshot* pStatus)
{
	if ( pStatus == NULL ) {
		return;
	}

	XS_LockCheckConfigState();
	memset(pStatus, 0, sizeof(XS_CheckConfigStatusSnapshot));
	pStatus->HasResult = g_bXsCheckConfigHasResult;
	pStatus->LastResult = g_bXsCheckConfigLastResult;
	pStatus->LastTime = g_tXsCheckConfigLastTime;
	pStatus->iTotalCount = XS_HttpMetricGet(&g_iXsCheckConfigTotalCount);
	pStatus->iSuccessCount = XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount);
	pStatus->iFailureCount = XS_HttpMetricGet(&g_iXsCheckConfigFailureCount);
	pStatus->iLastServerCount = g_iXsCheckConfigLastServerCount;
	if ( g_sXsCheckConfigLastFile[0] ) {
		strncpy(pStatus->sLastFile, g_sXsCheckConfigLastFile, sizeof(pStatus->sLastFile) - 1);
	}
	if ( g_sXsCheckConfigLastBase[0] ) {
		strncpy(pStatus->sLastBase, g_sXsCheckConfigLastBase, sizeof(pStatus->sLastBase) - 1);
	}
	if ( g_sXsCheckConfigLastMessage[0] ) {
		strncpy(pStatus->sLastMessage, g_sXsCheckConfigLastMessage, sizeof(pStatus->sLastMessage) - 1);
	}
	XS_UnlockCheckConfigState();
}

static inline void XS_GetReloadStatusSnapshot(XS_ReloadStatusSnapshot* pStatus)
{
	XS_ConfigReloadStatus tReloadStatus;

	if ( pStatus == NULL ) {
		return;
	}

	memset(pStatus, 0, sizeof(XS_ReloadStatusSnapshot));
	memset(&tReloadStatus, 0, sizeof(tReloadStatus));
	XS_GetConfigReloadStatusSnapshot(&tReloadStatus);

	pStatus->Busy = tReloadStatus.Busy;
	pStatus->Success = tReloadStatus.Success;
	pStatus->HasResult = tReloadStatus.HasResult;
	pStatus->LastTime = tReloadStatus.LastTime;
	pStatus->iTotalCount = tReloadStatus.iTotalCount;
	pStatus->iSuccessCount = tReloadStatus.iSuccessCount;
	pStatus->iFailureCount = tReloadStatus.iFailureCount;
	if ( tReloadStatus.sServerName[0] ) {
		strncpy(pStatus->sServerName, tReloadStatus.sServerName, sizeof(pStatus->sServerName) - 1);
	}
	if ( tReloadStatus.sHostName[0] ) {
		strncpy(pStatus->sHostName, tReloadStatus.sHostName, sizeof(pStatus->sHostName) - 1);
	}
	if ( tReloadStatus.sMessage[0] ) {
		strncpy(pStatus->sMessage, tReloadStatus.sMessage, sizeof(pStatus->sMessage) - 1);
	}
}

static inline void XS_RecordReloadStatusLite(const char* sServerName, const char* sHostName, bool bForce, int iCode)
{
	XS_ConfigReloadRequest tReq;
	XS_ConfigReloadStatus tBusyStatus;
	bool bPreserveBusy;

	memset(&tReq, 0, sizeof(tReq));
	memset(&tBusyStatus, 0, sizeof(tBusyStatus));
	tReq.Force = bForce;
	if ( sServerName && sServerName[0] ) {
		strncpy(tReq.sServerName, sServerName, sizeof(tReq.sServerName) - 1);
	}
	if ( sHostName && sHostName[0] ) {
		strncpy(tReq.sHostName, sHostName, sizeof(tReq.sHostName) - 1);
	}

	XS_GetConfigReloadStatusSnapshot(&tBusyStatus);
	bPreserveBusy = tBusyStatus.Busy;
	XS_SetConfigReloadStatusEx((iCode == 0), &tReq, XS_ReloadResultText(iCode), bPreserveBusy ? &tBusyStatus : NULL);
}

static inline void XS_UpdateRuntimeStats(XS_Runtime* objRuntime)
{
	if ( objRuntime && objRuntime->pEngine ) {
		g_iXsEngineWorkers = xrtNetEngineGetWorkerCount(objRuntime->pEngine);
	} else {
		g_iXsEngineWorkers = 0;
	}
	if ( objRuntime && objRuntime->Servers ) {
		g_iXsRuntimeServerCount = objRuntime->Servers->Count;
	} else {
		g_iXsRuntimeServerCount = 0;
	}
}
