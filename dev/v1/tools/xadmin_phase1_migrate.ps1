$ErrorActionPreference = 'Stop'

function procReadText936
{
	param(
		[string]$sPath
	)

	$objEncoding = [System.Text.Encoding]::GetEncoding(936)
	return [System.IO.File]::ReadAllText($sPath, $objEncoding)
}

function procWriteText936
{
	param(
		[string]$sPath,
		[string]$sText
	)

	$objEncoding = [System.Text.Encoding]::GetEncoding(936)
	[System.IO.File]::WriteAllText($sPath, $sText, $objEncoding)
}

function procReplaceText936
{
	param(
		[string]$sPath,
		[hashtable]$tblReplace
	)

	$sOld = procReadText936 $sPath
	$sNew = $sOld

	foreach ( $objKey in $tblReplace.Keys ) {
		$sNew = $sNew.Replace([string]$objKey, [string]$tblReplace[$objKey])
	}

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procSyncRuntime
{
	param(
		[string]$sXsRoot,
		[string]$sAdminRoot
	)

	Copy-Item -Force (Join-Path $sXsRoot 'release\xs') (Join-Path $sAdminRoot 'xs')
	Copy-Item -Force (Join-Path $sXsRoot 'release\xs.exe') (Join-Path $sAdminRoot 'xs.exe')

	if ( Test-Path (Join-Path $sAdminRoot 'tcc') ) {
		Remove-Item -Recurse -Force (Join-Path $sAdminRoot 'tcc')
	}

	Copy-Item -Recurse -Force (Join-Path $sXsRoot 'release\tcc') (Join-Path $sAdminRoot 'tcc')
}

function procUpdateXSJson
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'xs.json'
	$sJSON = @'
[
	{
		"enabled": true,
		"class": "http",
		"name": "xywhsoft Server",
		"desc": "xywhsoft server",
		"param": "",
		"backlog": 128,
		"recv_limit": 1048576,
		"path_limit": 200,
		"header_limit": 32,
		"body_limit": 262144,
		"ip": "0.0.0.0",
		"port": 80,
		"tls": true,
		"port_tls": 443,
		"host_default": {
			"enabled": true,
			"name": "xAdmin",
			"desc": "xadmin host",
			"param": "",
			"debug": false,
			"path": "host/xadmin/wwwroot",
			"tls_ca": "host/xadmin/tls/ca.pem",
			"tls_cert": "host/xadmin/tls/cert.pem",
			"tls_key": "host/xadmin/tls/key.pem",
			"devlang": "c",
			"devfile": "host/xadmin/script/main.c"
		},
		"hosts": []
	}
]
'@

	[System.IO.File]::WriteAllText($sPath, $sJSON, [System.Text.UTF8Encoding]::new($false))
}

function procRewriteDBModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\db.h'
	$sText = @'


// init db
void DB_Init()
{
	int iRet;
	str sFile;

	printf("        DB_Init \n");
	sFile = xrtPathJoin(2, DBPath, "main.db");
	iRet = sqlite3_open(sFile, &G_DB);
	xrtFree(sFile);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! ServiceInit - sqlite3_open error.\n");
		if ( G_DB ) {
			printf("%s\n", sqlite3_errmsg(G_DB));
			sqlite3_close(G_DB);
			G_DB = NULL;
		}
		exit(1);
	}
}



// free db
void DB_Unit()
{
	printf("        DB_Unit \n");
	if ( G_DB ) {
		sqlite3_close(G_DB);
		G_DB = NULL;
	}
}

'@

	procWriteText936 $sPath $sText
}

function procReplaceDBTypes
{
	param(
		[string]$sAdminRoot
	)

	$arrFiles = Get-ChildItem (Join-Path $sAdminRoot 'host\xadmin\script') -Recurse -File
	$tblReplace = @{
		'XDO_Connect' = 'sqlite3*'
		'->objDB' = ''
	}

	foreach ( $objFile in $arrFiles ) {
		procReplaceText936 $objFile.FullName $tblReplace
	}
}

function procRewriteDefineHash
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\define.h'
	$sOld = procReadText936 $sPath
	$sNew = [System.Text.RegularExpressions.Regex]::Replace(
		$sOld,
		'str ServerHashPassword\(str user, str salt, str clientHash\)\s*\{.*?\n\}',
@'
str ServerHashPassword(str user, str salt, str clientHash)
{
	str sCombined = xrtFormat("%s%s%s", user, salt, clientHash);
	uint8 arrHash[32];
	str sPwdHash;

	xrtSHA256((const ptr)sCombined, strlen(sCombined), arrHash);
	sPwdHash = xrtHexEncode(arrHash, sizeof(arrHash));
	xrtFree(sCombined);
	return sPwdHash;
}
'@,
		[System.Text.RegularExpressions.RegexOptions]::Singleline
	)

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewriteGuardModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\guard.h'
	$sText = @'


// brute-force guard
typedef struct {
	xtime CoolDown;
	int FailCount;
	int TimeRate;
} GuardInfo;
xdict G_BruteGuard = NULL;

// after how many failures enter cooldown
#define BRUTE_COUNT 5

// initial cooldown seconds
#define BRUTE_TIMES 300



static str Guard_RemoteKey(str sRemote)
{
	if ( sRemote == NULL || sRemote[0] == '\0' ) {
		return "(unknown)";
	}

	return sRemote;
}



xtime Guard_Check(str sRemote)
{
	GuardInfo* pInfo;
	bool bNew = FALSE;
	str sKey = Guard_RemoteKey(sRemote);

	pInfo = xrtDictSet(G_BruteGuard, sKey, strlen(sKey), &bNew);
	if ( pInfo == NULL ) {
		return 0;
	}
	if ( bNew ) {
		pInfo->FailCount = 0;
		pInfo->CoolDown = 0;
		pInfo->TimeRate = 0;
		return 0;
	}
	if ( pInfo->CoolDown > xrtNow() ) {
		return pInfo->CoolDown;
	}

	return 0;
}



void Guard_Failed(str sRemote)
{
	GuardInfo* pInfo;
	str sKey = Guard_RemoteKey(sRemote);

	pInfo = xrtDictSet(G_BruteGuard, sKey, strlen(sKey), NULL);
	if ( pInfo == NULL ) {
		return;
	}

	pInfo->FailCount++;
	if ( pInfo->FailCount >= BRUTE_COUNT ) {
		pInfo->FailCount = 0;
		pInfo->TimeRate++;
		pInfo->CoolDown = xrtNow() + (BRUTE_TIMES * pInfo->TimeRate);
	}
}



void Guard_Reset(str sRemote)
{
	GuardInfo* pInfo;
	str sKey = Guard_RemoteKey(sRemote);

	pInfo = xrtDictSet(G_BruteGuard, sKey, strlen(sKey), NULL);
	if ( pInfo == NULL ) {
		return;
	}

	pInfo->FailCount = 0;
	pInfo->CoolDown = 0;
	pInfo->TimeRate = 0;
}



void Guard_Init()
{
	printf("        Guard_Init \n");
	G_BruteGuard = xrtDictCreate(sizeof(GuardInfo));
}



void Guard_Unit()
{
	printf("        Guard_Unit \n");
	xrtDictDestroy(G_BruteGuard);
}

'@

	procWriteText936 $sPath $sText
}

function procRewriteMainIncludes
{
	param(
		[string]$sAdminRoot
	)

	$arrFiles = Get-ChildItem (Join-Path $sAdminRoot 'host\xadmin\script') -Recurse -File

	foreach ( $objFile in $arrFiles ) {
		$sOld = procReadText936 $objFile.FullName
		$sNew = $sOld -replace '<xsbase\.h>', '<xs_vnext_full.h>'

		if ( $objFile.FullName -like '*\host\xadmin\script\main.c' ) {
			$sNew = $sNew.Replace("#include <inline_other.h>`r`n", '')
			$sNew = $sNew.Replace("#include <inline_other.h>`n", '')
		}

		if ( $sNew -ne $sOld ) {
			procWriteText936 $objFile.FullName $sNew
		}
	}
}

function procRewriteDefineModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\define.h'
	$sOld = procReadText936 $sPath
	$sNew = $sOld

	$sNew = $sNew.Replace(
		'void (*Proc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm);',
		'void (*Proc)(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp, xvalue objSession);'
	)

	if ( $sNew.IndexOf('bool HttpMethodIs(XS_RequestObject objReq, const char* sMethod);') -lt 0 ) {
		$sInsert = @'

  // xadmin http helper api
  typedef struct HttpMultipartPart {
	  const char* sName;
	  size_t iNameLen;
	  const char* sFileName;
	  size_t iFileNameLen;
	  const char* pBody;
	  size_t iBodyLen;
  } HttpMultipartPart;
  bool HttpMethodIs(XS_RequestObject objReq, const char* sMethod);
  size_t HttpMethodLen(XS_RequestObject objReq);
  size_t HttpPathLen(XS_RequestObject objReq);
  size_t HttpQueryLen(XS_RequestObject objReq);
  int HttpGetQueryVar(XS_RequestObject objReq, const char* sName, char* sOut, size_t iOutCap);
  bool HttpMultipartNameIs(const HttpMultipartPart* pPart, const char* sName);
  bool HttpMultipartNext(XS_RequestObject objReq, size_t* pOffset, HttpMultipartPart* pPart);
  int http_reply(XS_ResponseObject objResp, int iCode, str sHead, const void* pBody, size_t iLen);
  int mg_http_reply(XS_ResponseObject objResp, int iCode, str sHead, str sFormat, ...);
  void LoadPage(XS_ResponseObject objResp, int iCode, str sHead, str sPage);
  TCCState* xsCreateTCC(const char* sWorkPath);
  void xsDestroyTCC(TCCState* s);
  
'@
		$sNew = $sNew.Replace('xvalue G_CACHE_RoleAuth = NULL;', $sInsert + 'xvalue G_CACHE_RoleAuth = NULL;')
	}

	if ( $sNew.IndexOf('TCCState* xsCreateTCC(const char* sWorkPath);') -lt 0 ) {
		$sTCCInsert = @'
TCCState* xsCreateTCC(const char* sWorkPath);
void xsDestroyTCC(TCCState* s);
'@
		$sNew = $sNew.Replace('xvalue G_CACHE_RoleAuth = NULL;', $sTCCInsert + "`r`n" + 'xvalue G_CACHE_RoleAuth = NULL;')
	}

	if ( $sNew.IndexOf('typedef struct HttpMultipartPart {') -lt 0 ) {
		$sMultipartInsert = @'
typedef struct HttpMultipartPart {
	const char* sName;
	size_t iNameLen;
	const char* sFileName;
	size_t iFileNameLen;
	const char* pBody;
	size_t iBodyLen;
} HttpMultipartPart;
bool HttpMultipartNameIs(const HttpMultipartPart* pPart, const char* sName);
bool HttpMultipartNext(XS_RequestObject objReq, size_t* pOffset, HttpMultipartPart* pPart);
'@
		$sNew = $sNew.Replace('bool HttpMethodIs(XS_RequestObject objReq, const char* sMethod);', $sMultipartInsert + "`r`n" + 'bool HttpMethodIs(XS_RequestObject objReq, const char* sMethod);')
	}

	$sNew = $sNew.Replace('ExePath = xCore->AppPath;', 'ExePath = (str)xsAppPath();')
	$sNew = $sNew.Replace('WebPath = objHost->Path;', 'WebPath = (str)xsHostPath(objHost);')

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewritePageModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\page.h'
	$sText = @'


// load static page from data/page
void LoadPage(XS_ResponseObject objResp, int iCode, str sHead, str sPage)
{
	str sFile = xrtPathJoin(2, PagePath, sPage);
	size_t iRetSize = 0;
	str sHTML = xrtFileGetAll(sFile, &iRetSize);

	if ( sHTML == NULL ) {
		http_reply(objResp, 404, HTTP_CT_HTML, "<h1>404</h1>", 0);
		xrtFree(sFile);
		return;
	}

	http_reply(objResp, iCode, sHead, sHTML, iRetSize);
	xrtFree(sHTML);
	xrtFree(sFile);
}

'@

	procWriteText936 $sPath $sText
}

function procRewriteLogsModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\logs.h'
	$sText = @'


// prepared sql
sqlite3_stmt* stmt_logs_all = NULL;
sqlite3_stmt* stmt_logs_sel = NULL;
sqlite3_stmt* stmt_logs_add = NULL;
sqlite3_stmt* stmt_logs_clear = NULL;



// init logs module
void Logs_Init()
{
	printf("        Logs_Init \n");

	int iRet = sqlite3_prepare_v3(G_DB, "SELECT *, COUNT(*) OVER() AS total_count FROM logs ORDER BY id DESC LIMIT ?  OFFSET ?;", -1, SQL_PREPARE_DEFAULT, &stmt_logs_all, NULL);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! Logs_Init [stmt_logs_all] - sqlite3_prepare_v3 error code : %d\n%s\n", iRet, sqlite3_errmsg(G_DB));
		exit(0);
	}
	iRet = sqlite3_prepare_v3(G_DB, "SELECT *, COUNT(*) OVER() AS total_count FROM logs WHERE uri LIKE ? ORDER BY id DESC LIMIT ?  OFFSET ?;", -1, SQL_PREPARE_DEFAULT, &stmt_logs_sel, NULL);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! Logs_Init [stmt_logs_sel] - sqlite3_prepare_v3 error code : %d\n%s\n", iRet, sqlite3_errmsg(G_DB));
		exit(0);
	}
	iRet = sqlite3_prepare_v3(G_DB, "INSERT INTO logs (user, ip, uri, method, param, body, createTime) VALUES (?, ?, ?, ?, ?, ?, ?);", -1, SQL_PREPARE_DEFAULT, &stmt_logs_add, NULL);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! Logs_Init [stmt_logs_add] - sqlite3_prepare_v3 error code : %d\n%s\n", iRet, sqlite3_errmsg(G_DB));
		exit(0);
	}
	iRet = sqlite3_prepare_v3(G_DB, "DELETE FROM logs WHERE createTime < ?;", -1, SQL_PREPARE_DEFAULT, &stmt_logs_clear, NULL);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! Logs_Init [stmt_logs_clear] - sqlite3_prepare_v3 error code : %d\n%s\n", iRet, sqlite3_errmsg(G_DB));
		exit(0);
	}
}



// add access log
void Logs_Add(XS_RequestObject objReq, xvalue objSession)
{
	const char* sUser = "(guest)";
	const char* sIP = xsReqRemote(objReq);
	const char* sURI = xsReqPath(objReq);
	const char* sQuery = xsReqQuery(objReq);
	const char* sMethod = xsReqMethod(objReq);
	const char* pBody = NULL;
	size_t iBodyLen = 0;
	xtime now = xrtNow();

	if ( objSession && (objSession->Type == XVO_DT_TABLE) ) {
		sUser = xvoTableGetText(objSession, "user", 4);
		if ( !sUser ) {
			sUser = "(unknown)";
		}
	}
	if ( sIP == NULL || sIP[0] == '\0' ) {
		sIP = "(unknown)";
	}
	if ( sURI == NULL ) {
		sURI = "";
	}
	if ( sQuery == NULL ) {
		sQuery = "";
	}
	if ( sMethod == NULL ) {
		sMethod = "";
	}
	if ( HttpMethodIs(objReq, "POST") || HttpMethodIs(objReq, "PUT") ) {
		pBody = (const char*)xsReqBody(objReq);
		iBodyLen = xsReqBodyLen(objReq);
	}
	if ( pBody == NULL ) {
		pBody = "";
		iBodyLen = 0;
	}

	sqlite3_bind_text(stmt_logs_add, 1, sUser, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt_logs_add, 2, sIP, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt_logs_add, 3, sURI, (int)HttpPathLen(objReq), SQLITE_STATIC);
	sqlite3_bind_text(stmt_logs_add, 4, sMethod, (int)HttpMethodLen(objReq), SQLITE_STATIC);
	sqlite3_bind_text(stmt_logs_add, 5, sQuery, (int)HttpQueryLen(objReq), SQLITE_STATIC);
	sqlite3_bind_text(stmt_logs_add, 6, pBody, (int)iBodyLen, SQLITE_STATIC);
	sqlite3_bind_int64(stmt_logs_add, 7, now);
	sqlite3_step(stmt_logs_add);
	sqlite3_reset(stmt_logs_add);
}



// free logs module
void Logs_Unit()
{
	printf("        Logs_Unit \n");
	sqlite3_finalize(stmt_logs_all);
	sqlite3_finalize(stmt_logs_sel);
	sqlite3_finalize(stmt_logs_add);
	sqlite3_finalize(stmt_logs_clear);
}

'@

	procWriteText936 $sPath $sText
}

function procRewriteHTTPModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\http.h'
	$sText = @'

#include <stdarg.h>



static str HttpDupSpan(const char* sText, size_t iLen)
{
	str sOut;

	sOut = xrtMalloc(iLen + 1);
	if ( sOut == NULL ) {
		return NULL;
	}
	if ( (sText != NULL) && (iLen > 0) ) {
		memcpy(sOut, sText, iLen);
	}
	sOut[iLen] = '\0';
	return sOut;
}



static const char* HttpReasonText(uint32 iCode)
{
	switch ( iCode ) {
		case 200: return "OK";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		default: return "OK";
	}
}



bool HttpMethodIs(XS_RequestObject objReq, const char* sMethod)
{
	const char* sReqMethod = xsReqMethod(objReq);

	if ( sReqMethod == NULL || sMethod == NULL ) {
		return FALSE;
	}

	return strcmp(sReqMethod, sMethod) == 0;
}



size_t HttpMethodLen(XS_RequestObject objReq)
{
	const char* sText = xsReqMethod(objReq);

	return sText ? strlen(sText) : 0;
}



size_t HttpPathLen(XS_RequestObject objReq)
{
	const char* sText = xsReqPath(objReq);

	return sText ? strlen(sText) : 0;
}



size_t HttpQueryLen(XS_RequestObject objReq)
{
	const char* sText = xsReqQuery(objReq);

	return sText ? strlen(sText) : 0;
}



static int HttpCopyQueryValue(const char* sQuery, const char* sName, char* sOut, size_t iOutCap)
{
	xrtquerypair tPair;
	str sDecoded = NULL;
	size_t iDecodedLen;

	if ( sOut == NULL || iOutCap == 0 ) {
		return -1;
	}

	sOut[0] = '\0';
	if ( sQuery == NULL || sName == NULL ) {
		return -1;
	}
	if ( !xrtQueryFind(sQuery, sName, &tPair) ) {
		return -1;
	}

	sDecoded = xrtUrlDecode(tPair.tValue.sPtr, tPair.tValue.iLen);
	if ( sDecoded == NULL ) {
		sDecoded = HttpDupSpan("", 0);
		if ( sDecoded == NULL ) {
			return -1;
		}
	}

	iDecodedLen = strlen(sDecoded);
	if ( iDecodedLen >= iOutCap ) {
		iDecodedLen = iOutCap - 1;
	}
	if ( iDecodedLen > 0 ) {
		memcpy(sOut, sDecoded, iDecodedLen);
	}
	sOut[iDecodedLen] = '\0';
	xrtFree(sDecoded);
	return (int)iDecodedLen;
}



int HttpGetQueryVar(XS_RequestObject objReq, const char* sName, char* sOut, size_t iOutCap)
{
	return HttpCopyQueryValue(xsReqQuery(objReq), sName, sOut, iOutCap);
}



static bool HttpCookieFreeWalkProc(Dict_Key* pKey, ptr pVal, ptr pArg)
{
	char** psText = (char**)pVal;

	(void)pKey;
	(void)pArg;

	if ( psText && *psText ) {
		xrtFree(*psText);
	}

	return TRUE;
}



xdict ParseCookies(XS_RequestObject objReq)
{
	const char* sCookie = xsReqHeader(objReq, "Cookie");
	xdict tblCookie = xrtDictCreate(sizeof(char*));
	size_t iOffset = 0;
	xrtcookiepair tCookie;

	if ( sCookie == NULL ) {
		return tblCookie;
	}

	while ( xrtCookieNext(sCookie, &iOffset, &tCookie) ) {
		char** psValue;
		str sDecoded;

		psValue = xrtDictSet(tblCookie, tCookie.tName.sPtr, tCookie.tName.iLen, NULL);
		if ( psValue == NULL ) {
			continue;
		}

		sDecoded = xrtUrlDecode(tCookie.tValue.sPtr, tCookie.tValue.iLen);
		if ( sDecoded == NULL ) {
			sDecoded = HttpDupSpan("", 0);
		}
		*psValue = sDecoded;
	}

	return tblCookie;
}



void FreeCookies(xdict tblCookies)
{
	if ( tblCookies == NULL ) {
		return;
	}

	xrtDictWalk(tblCookies, HttpCookieFreeWalkProc, NULL);
	xrtDictDestroy(tblCookies);
}



static void HttpApplyHeaderLine(XS_ResponseObject objResp, const char* sLine, size_t iLineLen, const char** psContentType, str* psContentTypeMem)
{
	const char* sColon;
	size_t iNameLen;
	size_t iValueOff;
	size_t iValueLen;

	sColon = (const char*)memchr(sLine, ':', iLineLen);
	if ( sColon == NULL ) {
		return;
	}

	iNameLen = (size_t)(sColon - sLine);
	iValueOff = iNameLen + 1;
	while ( iValueOff < iLineLen && (sLine[iValueOff] == ' ' || sLine[iValueOff] == '\t') ) {
		iValueOff++;
	}
	iValueLen = iLineLen - iValueOff;
	while ( iValueLen > 0 && (sLine[iValueOff + iValueLen - 1] == ' ' || sLine[iValueOff + iValueLen - 1] == '\t') ) {
		iValueLen--;
	}

	if ( iNameLen == 12 && strncmp(sLine, "Content-Type", 12) == 0 ) {
		if ( *psContentTypeMem ) {
			xrtFree(*psContentTypeMem);
			*psContentTypeMem = NULL;
		}
		*psContentTypeMem = HttpDupSpan(sLine + iValueOff, iValueLen);
		if ( *psContentTypeMem ) {
			*psContentType = *psContentTypeMem;
		}
		return;
	}

	str sName = HttpDupSpan(sLine, iNameLen);
	str sValue = HttpDupSpan(sLine + iValueOff, iValueLen);
	if ( sName && sValue ) {
		xsHttpHeader(objResp, sName, sValue);
	}
	if ( sName ) {
		xrtFree(sName);
	}
	if ( sValue ) {
		xrtFree(sValue);
	}
}



static void HttpApplyHeaders(XS_ResponseObject objResp, const char* sHead, const char** psContentType, str* psContentTypeMem)
{
	const char* sLine;

	if ( sHead == NULL || sHead[0] == '\0' ) {
		return;
	}

	sLine = sHead;
	while ( *sLine ) {
		const char* sNext = strstr(sLine, "\r\n");
		size_t iLineLen;

		if ( sNext ) {
			iLineLen = (size_t)(sNext - sLine);
		} else {
			iLineLen = strlen(sLine);
		}

		if ( iLineLen == 0 ) {
			break;
		}

		HttpApplyHeaderLine(objResp, sLine, iLineLen, psContentType, psContentTypeMem);
		if ( sNext == NULL ) {
			break;
		}
		sLine = sNext + 2;
	}
}



int http_reply(XS_ResponseObject objResp, int iCode, str sHead, const void* pBody, size_t iLen)
{
	const char* sContentType = "text/plain; charset=utf-8";
	str sContentTypeMem = NULL;
	const char* pOutBody = (const char*)pBody;

	if ( objResp == NULL ) {
		return 0;
	}

	xsHttpStatus(objResp, (uint32)iCode, HttpReasonText((uint32)iCode));
	HttpApplyHeaders(objResp, sHead, &sContentType, &sContentTypeMem);

	if ( pOutBody == NULL ) {
		pOutBody = "";
		iLen = 0;
	} else if ( iLen == 0 ) {
		iLen = strlen(pOutBody);
	}

	int iRet = xsHttpBody(objResp, pOutBody, iLen, sContentType);
	if ( sContentTypeMem ) {
		xrtFree(sContentTypeMem);
	}
	return iRet;
}



int mg_http_reply(XS_ResponseObject objResp, int iCode, str sHead, str sFormat, ...)
{
	va_list objArgs;
	int iLen;
	str sText;
	int iRet;

	if ( sFormat == NULL ) {
		return http_reply(objResp, iCode, sHead, "", 0);
	}

	va_start(objArgs, sFormat);
#if defined(_WIN32) || defined(_WIN64)
	iLen = _vscprintf(sFormat, objArgs);
#else
	iLen = vsnprintf(NULL, 0, sFormat, objArgs);
#endif
	va_end(objArgs);
	if ( iLen < 0 ) {
		return http_reply(objResp, 500, HTTP_CT_TEXT, "format failed", 0);
	}

	sText = xrtMalloc((size_t)iLen + 1);
	if ( sText == NULL ) {
		return http_reply(objResp, 500, HTTP_CT_TEXT, "alloc failed", 0);
	}

	va_start(objArgs, sFormat);
	vsnprintf(sText, (size_t)iLen + 1, sFormat, objArgs);
	va_end(objArgs);

	iRet = http_reply(objResp, iCode, sHead, sText, (size_t)iLen);
	xrtFree(sText);
	return iRet;
}



// admin request auth
static void AdminRequestAuth(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp, xvalue objSession, RouteInfo* pInfo)
{
	if ( objSession->Type == XVO_DT_TABLE ) {
		int64 iRoleID = xvoTableGetInt(objSession, "roleID", 6);
		xvalue tblRole = xvoListGetValue(G_CACHE_RoleAuth, iRoleID);
		if ( tblRole && (tblRole->Type == XVO_DT_TABLE) ) {
			bool bOK = xvoTableGetBool(tblRole, xsReqPath(objReq), (int)HttpPathLen(objReq));
			if ( bOK ) {
				if ( pInfo->bActive ) {
					Session_ExtendAdmin(objSession);
				}
				pInfo->Proc(objServer, objHost, objReq, objResp, objSession);
			} else {
				LoadPage(objResp, 403, HTTP_CT_HTML, "status/403.html");
			}
		} else {
			LoadPage(objResp, 403, HTTP_CT_HTML, "status/403.html");
		}
	} else {
		http_reply(objResp, 302, "Location: /admin/login\r\n", NULL, 0);
	}
}



// member request auth
static void MemberRequestAuth(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp, xvalue objSession, RouteInfo* pInfo)
{
	if ( objSession->Type == XVO_DT_TABLE ) {
		int64 iGroupID = xvoTableGetInt(objSession, "groupId", 7);
		xvalue tblGroup = xvoListGetValue(G_CACHE_MemberGroupAuth, iGroupID);
		if ( tblGroup && (tblGroup->Type == XVO_DT_TABLE) ) {
			bool bOK = xvoTableGetBool(tblGroup, xsReqPath(objReq), (int)HttpPathLen(objReq));
			if ( bOK ) {
				if ( pInfo->bActive ) {
					Session_ExtendMember(objSession);
				}
				pInfo->Proc(objServer, objHost, objReq, objResp, objSession);
			} else {
				http_reply(objResp, 403, HTTP_CT_JSON, "{\"code\":403,\"msg\":\"权限不足\"}", 0);
			}
		} else {
			http_reply(objResp, 403, HTTP_CT_JSON, "{\"code\":403,\"msg\":\"权限不足\"}", 0);
		}
	} else {
		http_reply(objResp, 401, HTTP_CT_JSON, "{\"code\":401,\"msg\":\"未登录\"}", 0);
	}
}



// current http request dispatcher
bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath = xsReqPath(objReq);
	RouteInfo* pInfo;
	xdict tblCookies = NULL;
	xvalue objSession = NULL;
	bool bOwnSession = FALSE;
	str sSessionID;

	if ( sPath == NULL || sPath[0] == '\0' ) {
		return FALSE;
	}

	pInfo = xrtDictGet(G_StaticRouteTableHTTP, sPath, strlen(sPath));
	if ( pInfo == NULL ) {
		return FALSE;
	}

	if ( G_Install == FALSE ) {
		objSession = xvoCreateNull();
		if ( objSession ) {
			bOwnSession = TRUE;
		}
		Request_Install(objServer, objHost, objReq, objResp, objSession);
		if ( bOwnSession && objSession ) {
			xvoUnref(objSession);
		}
		return TRUE;
	}

	tblCookies = ParseCookies(objReq);
	if ( pInfo->bAdmin ) {
		sSessionID = xrtDictGetPtr(tblCookies, "XSID", 4);
		if ( sSessionID ) {
			objSession = xvoTableGetValue(G_AdminSession, sSessionID, 0);
		}
	} else {
		sSessionID = xrtDictGetPtr(tblCookies, "MSID", 4);
		if ( sSessionID ) {
			objSession = xvoTableGetValue(G_MemberSession, sSessionID, 0);
		}
	}

	if ( objSession == NULL ) {
		objSession = xvoCreateNull();
		if ( objSession ) {
			bOwnSession = TRUE;
		}
	}

	if ( objSession && (objSession->Type == XVO_DT_TABLE) && Session_IsExpired(objSession) ) {
		if ( pInfo->bAdmin ) {
			xvoTableRemove(G_AdminSession, sSessionID, 0);
		} else {
			xvoTableRemove(G_MemberSession, sSessionID, 0);
		}
		if ( bOwnSession && objSession ) {
			xvoUnref(objSession);
		}
		objSession = xvoCreateNull();
		bOwnSession = TRUE;
	}

	if ( pInfo->bAuth ) {
		if ( pInfo->bAdmin ) {
			if ( pInfo->bPutLog ) {
				Logs_Add(objReq, objSession);
			}
			AdminRequestAuth(objServer, objHost, objReq, objResp, objSession, pInfo);
		} else {
			MemberRequestAuth(objServer, objHost, objReq, objResp, objSession, pInfo);
		}
	} else {
		pInfo->Proc(objServer, objHost, objReq, objResp, objSession);
	}

	FreeCookies(tblCookies);
	if ( bOwnSession && objSession ) {
		xvoUnref(objSession);
	}
	return TRUE;
}

'@

	procWriteText936 $sPath $sText
}

function procRewriteHTTPUsage
{
	param(
		[string]$sAdminRoot
	)

	$arrFiles = Get-ChildItem (Join-Path $sAdminRoot 'host\xadmin\script') -Recurse -File | Where-Object {
		$_.FullName -notlike '*\module\http.h' -and
		$_.FullName -notlike '*\module\logs.h' -and
		$_.FullName -notlike '*\module\page.h'
	}

	foreach ( $objFile in $arrFiles ) {
		$sOld = procReadText936 $objFile.FullName
		$sNew = $sOld

		$sNew = $sNew.Replace('struct mg_connection* c, struct mg_http_message* hm', 'XS_RequestObject objReq, XS_ResponseObject objResp, xvalue objSession')
		$sNew = $sNew.Replace('LoadPage(c,', 'LoadPage(objResp,')
		$sNew = $sNew.Replace('http_reply(c,', 'http_reply(objResp,')
		$sNew = $sNew.Replace('mg_http_reply(c,', 'mg_http_reply(objResp,')
		$sNew = $sNew.Replace('ParseCookies(hm)', 'ParseCookies(objReq)')
		$sNew = $sNew.Replace('Guard_Check(&c->rem)', 'Guard_Check(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Guard_Check(c->rem)', 'Guard_Check(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Guard_Failed(&c->rem)', 'Guard_Failed(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Guard_Failed(c->rem)', 'Guard_Failed(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Guard_Reset(&c->rem)', 'Guard_Reset(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Guard_Reset(c->rem)', 'Guard_Reset(xsReqRemote(objReq))')
		$sNew = $sNew.Replace('Logs_Add(c, hm)', 'Logs_Add(objReq, objSession)')
		$sNew = $sNew.Replace('hm->session', 'objSession')
		$sNew = $sNew.Replace('hm->body.buf', '(const char*)xsReqBody(objReq)')
		$sNew = $sNew.Replace('hm->body.len', 'xsReqBodyLen(objReq)')
		$sNew = $sNew.Replace('hm->query.buf', 'xsReqQuery(objReq)')
		$sNew = $sNew.Replace('hm->query.len', 'HttpQueryLen(objReq)')
		$sNew = $sNew.Replace('hm->uri.buf', 'xsReqPath(objReq)')
		$sNew = $sNew.Replace('hm->uri.len', 'HttpPathLen(objReq)')
		$sNew = $sNew.Replace('hm->method.buf', 'xsReqMethod(objReq)')
		$sNew = $sNew.Replace('hm->method.len', 'HttpMethodLen(objReq)')
		$sNew = $sNew.Replace('mg_http_get_var(&hm->query,', 'HttpGetQueryVar(objReq,')
		$sNew = $sNew.Replace('struct mg_http_part part;', 'HttpMultipartPart part;')
		$sNew = $sNew.Replace('part.body.buf', 'part.pBody')
		$sNew = $sNew.Replace('part.body.len', 'part.iBodyLen')
		$sNew = $sNew.Replace('part.filename.buf', 'part.sFileName')
		$sNew = $sNew.Replace('part.filename.len', 'part.iFileNameLen')

		$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, 'hm->methodCode\s*==\s*HTTP_([A-Z]+)', 'HttpMethodIs(objReq, "$1")')
		$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, 'hm->methodCode\s*!=\s*HTTP_([A-Z]+)', '!HttpMethodIs(objReq, "$1")')
		$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, 'while\s*\(\s*\(ofs\s*=\s*mg_http_next_multipart\(hm->body,\s*ofs,\s*&part\)\)\s*>\s*0\s*\)', 'while ( HttpMultipartNext(objReq, &ofs, &part) )')
		$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, 'mg_match\(part\.name,\s*mg_str\("([^"]+)"\),\s*NULL\)', 'HttpMultipartNameIs(&part, "$1")')

		if ( $sNew -ne $sOld ) {
			procWriteText936 $objFile.FullName $sNew
		}
	}
}

function procRewriteHTTPMultipartModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\http.h'
	$sOld = procReadText936 $sPath
	$sNew = $sOld

	$sNew = [System.Text.RegularExpressions.Regex]::Replace(
		$sNew,
		'typedef struct HttpMultipartPart \{.*?\} HttpMultipartPart;\s*',
		'',
		[System.Text.RegularExpressions.RegexOptions]::Singleline
	)

	if ( $sNew.IndexOf('bool HttpMultipartNameIs(const HttpMultipartPart* pPart, const char* sName)') -lt 0 ) {
		$sInsert = @'



bool HttpMultipartNameIs(const HttpMultipartPart* pPart, const char* sName)
{
	size_t iNameLen;

	if ( pPart == NULL || sName == NULL || pPart->sName == NULL ) {
		return FALSE;
	}

	iNameLen = strlen(sName);
	if ( pPart->iNameLen != iNameLen ) {
		return FALSE;
	}

	return memcmp(pPart->sName, sName, iNameLen) == 0;
}



bool HttpMultipartNext(XS_RequestObject objReq, size_t* pOffset, HttpMultipartPart* pPart)
{
	const char* sContentType;
	xrtstrview tBoundary;
	xrtmultipartpartview tPart;

	if ( pOffset == NULL || pPart == NULL ) {
		return FALSE;
	}

	memset(pPart, 0, sizeof(HttpMultipartPart));
	memset(&tBoundary, 0, sizeof(tBoundary));
	memset(&tPart, 0, sizeof(tPart));

	sContentType = xsReqHeader(objReq, "Content-Type");
	if ( sContentType == NULL || sContentType[0] == '\0' ) {
		return FALSE;
	}
	if ( !xrtMultipartBoundaryFromContentType(sContentType, &tBoundary) ) {
		return FALSE;
	}
	if ( !xrtMultipartNextN((const char*)xsReqBody(objReq), xsReqBodyLen(objReq), tBoundary.sPtr, tBoundary.iLen, pOffset, &tPart) ) {
		return FALSE;
	}

	pPart->sName = tPart.tName.sPtr;
	pPart->iNameLen = tPart.tName.iLen;
	pPart->sFileName = tPart.tFileName.sPtr;
	pPart->iFileNameLen = tPart.tFileName.iLen;
	pPart->pBody = tPart.tBody.sPtr;
	pPart->iBodyLen = tPart.tBody.iLen;
	return TRUE;
}

'@
		$sNew = $sNew.Replace('static bool HttpCookieFreeWalkProc(Dict_Key* pKey, ptr pVal, ptr pArg)', $sInsert + 'static bool HttpCookieFreeWalkProc(Dict_Key* pKey, ptr pVal, ptr pArg)')
	}

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewriteAttachmentUsage
{
	param(
		[string]$sAdminRoot
	)

	$arrFiles = @(
		(Join-Path $sAdminRoot 'host\xadmin\script\module\attachment.h')
		(Join-Path $sAdminRoot 'host\xadmin\script\plugin\attachment\main.c')
		(Join-Path $sAdminRoot 'host\xadmin\script\route_http\attachment.h')
		(Join-Path $sAdminRoot 'host\xadmin\script\route_http\attachment_api.h')
	)

	foreach ( $sPath in $arrFiles ) {
		if ( -not (Test-Path $sPath) ) {
			continue
		}

		$sOld = procReadText936 $sPath
		$sNew = $sOld

		$sNew = $sNew.Replace('bool Attachment_CheckHotlink(struct mg_http_message* hm, bool bAllowHotlink)', 'bool Attachment_CheckHotlink(XS_RequestObject objReq, bool bAllowHotlink)')
		$sNew = $sNew.Replace('bool CheckHotlink(struct mg_http_message* hm, bool bAllowHotlink)', 'bool CheckHotlink(XS_RequestObject objReq, bool bAllowHotlink)')
		$sNew = $sNew.Replace('struct mg_str* referer = mg_http_get_header(hm, "Referer");', 'const char* sReferer = xsReqHeader(objReq, "Referer");')
		$sNew = $sNew.Replace('if ( !referer || referer->len == 0 ) {', 'if ( sReferer == NULL || sReferer[0] == ''\0'' ) {')
		$sNew = $sNew.Replace('Attachment_ExtractHost(referer->buf, &iHostLen);', 'Attachment_ExtractHost((str)sReferer, &iHostLen);')
		$sNew = $sNew.Replace('ExtractHost(referer->buf, &iHostLen);', 'ExtractHost((str)sReferer, &iHostLen);')
		$sNew = $sNew.Replace('if ( mg_match(mg_str_n(sHost, iHostLen), mg_str("localhost"), NULL) ||', 'if ( ((iHostLen == 9) && (strncmp(sHost, "localhost", 9) == 0)) ||')
		$sNew = $sNew.Replace('		 mg_match(mg_str_n(sHost, iHostLen), mg_str("127.0.0.1"), NULL) ) {', '		 ((iHostLen == 9) && (strncmp(sHost, "127.0.0.1", 9) == 0)) ) {')
		$sNew = $sNew.Replace('Attachment_CheckHotlink(hm, iAllowHotlink)', 'Attachment_CheckHotlink(objReq, iAllowHotlink)')
		$sNew = $sNew.Replace('CheckHotlink(hm, iAllowHotlink)', 'CheckHotlink(objReq, iAllowHotlink)')

		if ( $sNew -ne $sOld ) {
			procWriteText936 $sPath $sNew
		}
	}
}

function procRewritePluginPageModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\plugin_page.h'
	$sOld = procReadText936 $sPath
	$sNew = $sOld

	$sNew = $sNew.Replace('void Plugin_LoadPage(struct mg_connection* c, int iCode, str sHead, str sPluginId, str sPage)', 'void Plugin_LoadPage(XS_ResponseObject objResp, int iCode, str sHead, str sPluginId, str sPage)')
	$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, '(?m)^(//[^\r\n]*?)(\b(?:void|int|str|bool)\s+[A-Za-z_][A-Za-z0-9_]*\s*\()', '$1' + "`r`n" + '$2')
	$sNew = [System.Text.RegularExpressions.Regex]::Replace($sNew, '(?m)^(//[^\r\n]*?)(\b(?:extern|static)\b[^\r\n;]+;)', '$1' + "`r`n" + '$2')

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewritePluginMgrModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\module\plugin_mgr.h'
	$sOld = procReadText936 $sPath
	$sNew = $sOld

	$tblReplace = @{
		'void PluginCtx_LoadPage(struct mg_connection* c, int code, str head, str pagePath);' = 'void PluginCtx_LoadPage(XS_ResponseObject objResp, int code, str head, str pagePath);'
		'void PluginCtx_SendJson(struct mg_connection* c, int code, str json, size_t len)' = 'void PluginCtx_SendJson(XS_ResponseObject objResp, int code, str json, size_t len)'
		'void PluginCtx_SendHtml(struct mg_connection* c, int code, str html)' = 'void PluginCtx_SendHtml(XS_ResponseObject objResp, int code, str html)'
		'void PluginCtx_SendPage(struct mg_connection* c, str pagePath, xvalue data)' = 'void PluginCtx_SendPage(XS_ResponseObject objResp, str pagePath, xvalue data)'
		'void PluginCtx_LoadPage(struct mg_connection* c, int code, str head, str pagePath)' = 'void PluginCtx_LoadPage(XS_ResponseObject objResp, int code, str head, str pagePath)'
		'void PluginCtx_SendFile(struct mg_connection* c, str filePath, str mimeType)' = 'void PluginCtx_SendFile(XS_ResponseObject objResp, str filePath, str mimeType)'
		'void PluginCtx_SendError(struct mg_connection* c, int code, str message)' = 'void PluginCtx_SendError(XS_ResponseObject objResp, int code, str message)'
	}

	foreach ( $objKey in $tblReplace.Keys ) {
		$sNew = $sNew.Replace([string]$objKey, [string]$tblReplace[$objKey])
	}

	if ( $sNew.IndexOf('void PluginCtx_SendJson(XS_ResponseObject objResp, int code, str json, size_t len);') -lt 0 ) {
		$sProtoInsert = @'
void PluginCtx_SendJson(XS_ResponseObject objResp, int code, str json, size_t len);
void PluginCtx_SendHtml(XS_ResponseObject objResp, int code, str html);
void PluginCtx_SendPage(XS_ResponseObject objResp, str pagePath, xvalue data);
void PluginCtx_SendFile(XS_ResponseObject objResp, str filePath, str mimeType);
void PluginCtx_SendError(XS_ResponseObject objResp, int code, str message);
'@
		$sNew = $sNew.Replace('void PluginCtx_LoadPage(XS_ResponseObject objResp, int code, str head, str pagePath);', 'void PluginCtx_LoadPage(XS_ResponseObject objResp, int code, str head, str pagePath);' + "`r`n" + $sProtoInsert)
	}

	$sNew = [System.Text.RegularExpressions.Regex]::Replace(
		$sNew,
		'void PluginCtx_SendFile\(XS_ResponseObject objResp, str filePath, str mimeType\)\s*\{.*?\n\}',
@'
void PluginCtx_SendFile(XS_ResponseObject objResp, str filePath, str mimeType)
{
	size_t iFileSize = 0;
	str sData = xrtFileGetAll(filePath, &iFileSize);
	str sHead = NULL;

	if ( sData == NULL ) {
		PluginCtx_SendError(objResp, 404, "File not found");
		return;
	}

	sHead = xrtFormat("Content-Type: %s\r\n", (mimeType && mimeType[0]) ? mimeType : "application/octet-stream");
	http_reply(objResp, 200, sHead, sData, iFileSize);
	xrtFree(sHead);
	xrtFree(sData);
}
'@,
		[System.Text.RegularExpressions.RegexOptions]::Singleline
	)

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewriteRouteLogsModule
{
	param(
		[string]$sAdminRoot
	)

	$sPath = Join-Path $sAdminRoot 'host\xadmin\script\route_http\logs.h'
	$sOld = procReadText936 $sPath
	$sNew = $sOld
	$arrLines = $sNew -split "`r?`n"

	for ( $i = 0; $i -lt $arrLines.Length; $i++ ) {
		if ( $arrLines[$i] -like '*xvoTableSetText(tblRet, "message", 7,*' ) {
			$arrLines[$i] = '		xvoTableSetText(tblRet, "message", 7, "日志数据获取成功！", 0, FALSE);'
		}
		if ( $arrLines[$i] -like '*http_reply(objResp, 200, HTTP_CT_JSON,*7*' ) {
			$arrLines[$i] = '		http_reply(objResp, 200, HTTP_CT_JSON, "{\"result\": true, \"message\": \"7天前的日志已清空！\"}", 0);'
		}
	}

	if ( $arrLines.Length -gt 90 ) {
		$arrLines[90] = '		xvoTableSetText(tblRet, "message", 7, "日志数据获取成功！", 0, FALSE);'
	}
	if ( $arrLines.Length -gt 122 ) {
		$arrLines[122] = '		http_reply(objResp, 200, HTTP_CT_JSON, "{\"result\": true, \"message\": \"7天前的日志已清空！\"}", 0);'
	}

	$sNew = [string]::Join("`r`n", $arrLines)

	if ( $sNew -ne $sOld ) {
		procWriteText936 $sPath $sNew
	}
}

function procRewriteXRTUsage
{
	param(
		[string]$sAdminRoot
	)

	$arrFiles = Get-ChildItem (Join-Path $sAdminRoot 'host\xadmin\script') -Recurse -File

	foreach ( $objFile in $arrFiles ) {
		$sOld = procReadText936 $objFile.FullName
		$sNew = $sOld

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtDictCreate\(sizeof\(([^,\)]+),\s*0\)\)',
			'xrtDictCreate(sizeof($1), 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtDictCreate\(sizeof\(([^,\)]+)\)\)',
			'xrtDictCreate(sizeof($1), 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtDictCreate\(\s*(0)\s*\)',
			'xrtDictCreate($1, 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtDictCreate\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)',
			'xrtDictCreate($1, 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtListCreate\(sizeof\(([^,\)]+),\s*0\)\)',
			'xrtListCreate(sizeof($1), 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtListCreate\(sizeof\(([^,\)]+)\)\)',
			'xrtListCreate(sizeof($1), 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtListCreate\(\s*(0)\s*\)',
			'xrtListCreate($1, 0)'
		)

		$sNew = [System.Text.RegularExpressions.Regex]::Replace(
			$sNew,
			'xrtListCreate\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)',
			'xrtListCreate($1, 0)'
		)

		if ( $sNew -ne $sOld ) {
			procWriteText936 $objFile.FullName $sNew
		}
	}
}

$sXsRoot = 'D:\Git\xserver'
$sAdminRoot = 'D:\Git\x-admin'

procSyncRuntime $sXsRoot $sAdminRoot
procUpdateXSJson $sAdminRoot
procReplaceDBTypes $sAdminRoot
procRewriteDBModule $sAdminRoot
procRewriteDefineHash $sAdminRoot
procRewriteGuardModule $sAdminRoot
procRewriteMainIncludes $sAdminRoot
procRewriteDefineModule $sAdminRoot
procRewritePageModule $sAdminRoot
procRewriteLogsModule $sAdminRoot
procRewriteHTTPModule $sAdminRoot
procRewriteHTTPMultipartModule $sAdminRoot
procRewriteHTTPUsage $sAdminRoot
procRewriteXRTUsage $sAdminRoot
procRewriteAttachmentUsage $sAdminRoot
procRewritePluginPageModule $sAdminRoot
procRewritePluginMgrModule $sAdminRoot
procRewriteRouteLogsModule $sAdminRoot
