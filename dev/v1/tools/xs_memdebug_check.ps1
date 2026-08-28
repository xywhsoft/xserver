param(
	[int]$iHttpCount = 12,
	[int]$iWsCount = 4,
	[int]$iXtpCount = 4,
	[int]$iCustomCount = 4,
	[int]$iWaitMS = 10000
)

$iManagePort = 8085
$iWsPort = 8081
$iXtpPort = 9096
$iCustomPort = 9098
$sRepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$sReleaseDir = Join-Path $sRepoRoot "release"
$sToolDir = Join-Path $sRepoRoot "tools"
$sManageUrl = "http://127.0.0.1:$iManagePort"
$sRunTag = [string]$PID
$sTempRoot = Join-Path $env:TEMP ("xs_memdebug_check_" + $sRunTag)
$script:sWsClientExe = $null
$script:sXtpClientExe = $null
$script:sCustomClientExe = $null
$script:sWinHelperExe = $null
$script:bKeepTemp = $false
$script:arrBaselineKeys = @(
	"live_alloc_count",
	"live_alloc_bytes",
	"foreign_live_count",
	"foreign_live_bytes",
	"live_object_count",
	"invalid_free_count",
	"double_free_count",
	"wrong_allocator_free_count",
	"object_double_destroy_count",
	"overflow_count",
	"underflow_count"
)

function procCleanupGeneratedFiles()
{
	foreach ( $sPattern in @(
		"ws_smoke_client.memdebug.*.exe",
		"xtp_pressure_client.memdebug.*.exe",
		"custom_smoke_client.memdebug.*.exe",
		"win_console_group_helper.memdebug.*.exe"
	) ) {
		Get-ChildItem -Path $sToolDir -Filter $sPattern -File -ErrorAction SilentlyContinue | ForEach-Object {
			Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
		}
	}
}

function procCleanupServers()
{
	Get-Process xs, xsdbg -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
	Start-Sleep -Milliseconds 500
}

function procGetFileHashText([string]$sPath)
{
	$hFile = $null
	$objSha = $null
	$arrHash = $null

	if ( -not (Test-Path $sPath) ) {
		return "(missing)"
	}

	try {
		$hFile = [System.IO.File]::OpenRead($sPath)
		$objSha = [System.Security.Cryptography.SHA256]::Create()
		$arrHash = $objSha.ComputeHash($hFile)
		return ([System.BitConverter]::ToString($arrHash)).Replace("-", "")
	} finally {
		if ( $null -ne $hFile ) {
			$hFile.Dispose()
		}
		if ( $null -ne $objSha ) {
			$objSha.Dispose()
		}
	}
}

function procFetchEx([string]$sMethod, [string]$sUrl)
{
	$iStatus = 0
	$sBody = ""
	$sBodyFile = [System.IO.Path]::GetTempFileName()
	$arrArgs = @("-s", "--max-time", "5", "-o", $sBodyFile, "-w", "%{http_code}")

	if ( $sMethod -ne "GET" ) {
		$arrArgs += @("-X", $sMethod)
	}

	$arrArgs += $sUrl

	try {
		$sStatus = (& curl.exe @arrArgs 2>$null) -join ""
		if ( Test-Path $sBodyFile ) {
			$sBody = [string](Get-Content -Raw -Path $sBodyFile -Encoding UTF8)
		}
		if ( ![int]::TryParse($sStatus.Trim(), [ref]$iStatus) ) {
			$iStatus = 0
		}
	} finally {
		if ( Test-Path $sBodyFile ) {
			Remove-Item $sBodyFile -Force -ErrorAction SilentlyContinue
		}
	}

	return @{
		status = $iStatus
		body = $sBody
	}
}

function procFetchJsonText([string]$sUrl)
{
	$objResp = procFetchEx "GET" $sUrl

	if ( $objResp.status -ne 200 ) {
		throw "fetch json failed: $sUrl status=$($objResp.status)"
	}
	if ( [string]::IsNullOrWhiteSpace($objResp.body) ) {
		throw "fetch json failed: $sUrl empty body"
	}

	return [string]$objResp.body
}

function procWaitReady()
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		try {
			$objResp = procFetchEx "GET" "$sManageUrl/__xs/status_json"
			if ( $objResp.status -eq 200 ) {
				return $true
			}
		} catch {
		}

		Start-Sleep -Milliseconds 200
	}

	return $false
}

function procWaitNotReady()
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		try {
			$objResp = procFetchEx "GET" "$sManageUrl/__xs/status_json"
			if ( $objResp.status -ne 200 ) {
				return $true
			}
		} catch {
			return $true
		}

		Start-Sleep -Milliseconds 200
	}

	return $false
}

function procWaitFile([string]$sPath)
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		if ( Test-Path $sPath ) {
			return $true
		}

		Start-Sleep -Milliseconds 200
	}

	return $false
}

function procBuildCClient([string]$sSourceName, [string]$sOutputBase, [string[]]$arrExtraArgs = @())
{
	$sSource = Join-Path $sToolDir $sSourceName
	$sOutput = Join-Path $sToolDir ("{0}.memdebug.{1}.exe" -f $sOutputBase, $sRunTag)
	$arrArgs = @($sSource, "-O2", "-s")
	$arrArgs += $arrExtraArgs
	$arrArgs += @("-o", $sOutput)

	if ( -not (Test-Path $sSource) ) {
		throw "$sSourceName not found"
	}

	& gcc @arrArgs | Out-Null
	if ( ($LASTEXITCODE -ne 0) -or (-not (Test-Path $sOutput)) ) {
		throw "build $sSourceName failed"
	}

	return $sOutput
}

function procBuildWinHelper()
{
	if ( $script:sWinHelperExe -and (Test-Path $script:sWinHelperExe) ) {
		return $script:sWinHelperExe
	}

	$script:sWinHelperExe = procBuildCClient "win_console_group_helper.c" "win_console_group_helper"
	return $script:sWinHelperExe
}

function procRunClientOnce([string]$sClientExe, [string[]]$arrArgs)
{
	$sOutput = & $sClientExe @arrArgs 2>&1
	$iCode = $LASTEXITCODE

	return @{
		code = $iCode
		text = [string]($sOutput -join "`n")
	}
}

function procRunClientRetry([string]$sClientExe, [string[]]$arrArgs, [int]$iRetryCount = 10)
{
	$tblRun = $null

	for ( $i = 0; $i -lt $iRetryCount; $i++ ) {
		$tblRun = procRunClientOnce $sClientExe $arrArgs
		if ( $tblRun.code -eq 0 ) {
			return $tblRun
		}

		if ( $i -lt ($iRetryCount - 1) ) {
			Start-Sleep -Milliseconds 200
		}
	}

	return $tblRun
}

function procRequire([bool]$bCondition, [string]$sMessage)
{
	if ( -not $bCondition ) {
		throw $sMessage
	}
}

function procRequireStatus($objResp, [int]$iExpect, [string]$sName)
{
	$sBody = ""

	if ( $null -ne $objResp -and $null -ne $objResp.body ) {
		$sBody = $objResp.body.Trim()
	}

	procRequire ($objResp.status -eq $iExpect) "$sName expected $iExpect, got $($objResp.status) body=$sBody"
}

function procGetMetricInt([string]$sMetricBody, [string]$sKey)
{
	if ( [string]::IsNullOrEmpty($sMetricBody) ) {
		throw "metric missing: $sKey"
	}

	if ( $sMetricBody -match ('"' + [Regex]::Escape($sKey) + '":\s*(-?\d+)') ) {
		return [long]$Matches[1]
	}

	throw "metric missing: $sKey"
}

function procGetMetricText([string]$sMetricBody, [string]$sKey)
{
	if ( [string]::IsNullOrEmpty($sMetricBody) ) {
		return ""
	}

	if ( $sMetricBody -match ('"' + [Regex]::Escape($sKey) + '":"((?:[^"\\]|\\.)*)"') ) {
		$sValue = $Matches[1]
		$sValue = $sValue.Replace('\/', '/')
		return [Regex]::Unescape($sValue)
	}

	return ""
}

function procGetReportSummary([string]$sPath)
{
	$objReport = Get-Content -Raw -Path $sPath -Encoding UTF8 | ConvertFrom-Json

	return [ordered]@{
		live_alloc_count = [long]$objReport.live_alloc_count
		live_alloc_bytes = [long]$objReport.live_alloc_bytes
		foreign_live_count = [long]$objReport.foreign_live_count
		foreign_live_bytes = [long]$objReport.foreign_live_bytes
		live_object_count = [long]$objReport.live_object_count
		invalid_free_count = [long]$objReport.invalid_free_count
		double_free_count = [long]$objReport.double_free_count
		wrong_allocator_free_count = [long]$objReport.wrong_allocator_free_count
		object_double_destroy_count = [long]$objReport.object_double_destroy_count
		overflow_count = [long]$objReport.overflow_count
		underflow_count = [long]$objReport.underflow_count
		live_allocations_count = @($objReport.live_allocations).Count
		foreign_live_allocations_count = @($objReport.foreign_live_allocations).Count
	}
}

function procGetCaseBaseline([string]$sCaseName)
{
	$tblBaseline = [ordered]@{
		live_alloc_count = 4
		live_alloc_bytes = 128
		live_object_count = 0
		invalid_free_count = 0
		double_free_count = 0
		wrong_allocator_free_count = 0
		object_double_destroy_count = 0
		overflow_count = 0
		underflow_count = 0
	}

	switch ( $sCaseName ) {
		"http" {
			$tblBaseline["foreign_live_count"] = 900
			$tblBaseline["foreign_live_bytes"] = 43200
		}
		"ws" {
			$tblBaseline["foreign_live_count"] = 840
			$tblBaseline["foreign_live_bytes"] = 40320
		}
		"xtp" {
			$tblBaseline["foreign_live_count"] = 840
			$tblBaseline["foreign_live_bytes"] = 40320
		}
		"custom" {
			$tblBaseline["foreign_live_count"] = 840
			$tblBaseline["foreign_live_bytes"] = 40320
		}
		default {
			throw "unknown memdebug baseline case: $sCaseName"
		}
	}

	return $tblBaseline
}

function procPrepareRunDir([string]$sCaseName)
{
	$sRunDir = Join-Path $sTempRoot $sCaseName
	Remove-Item $sRunDir -Recurse -Force -ErrorAction SilentlyContinue
	New-Item -ItemType Directory -Path $sRunDir | Out-Null
	return $sRunDir
}

function procStartServer([string]$sConfigName, [string]$sRunDir, [string]$sCaseName)
{
	$sHelperExe = procBuildWinHelper
	$sExePath = Join-Path $sReleaseDir "xsdbg.exe"
	$sConfigPath = Join-Path $sReleaseDir $sConfigName
	$sLogPath = Join-Path $sRunDir ($sCaseName + ".log")
	$sPidPath = Join-Path $sRunDir ($sCaseName + ".pid")
	$sPidText = ""
	$iPid = 0

	if ( -not (Test-Path $sExePath) ) {
		throw "xsdbg.exe not found"
	}
	if ( -not (Test-Path $sConfigPath) ) {
		throw "config not found: $sConfigName"
	}

	Remove-Item $sPidPath -Force -ErrorAction SilentlyContinue
	& $sHelperExe spawn $sRunDir $sLogPath $sPidPath $sExePath $sConfigPath 2>$null | Out-Null
	if ( $LASTEXITCODE -ne 0 ) {
		throw "spawn server failed: $sCaseName"
	}
	if ( -not (procWaitFile $sPidPath) ) {
		throw "spawn server pid file timeout: $sCaseName"
	}
	$sPidText = [string](Get-Content -Raw -Path $sPidPath -Encoding UTF8)
	if ( -not [int]::TryParse($sPidText.Trim(), [ref]$iPid) ) {
		throw "spawn server pid invalid: $sCaseName text=$sPidText"
	}
	if ( -not (procWaitReady) ) {
		Get-Process -Id $iPid -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
		throw "server ready timeout: $sCaseName"
	}

	return @{
		pid = $iPid
		run_dir = $sRunDir
		log_path = $sLogPath
		config = $sConfigName
	}
}

function procStopServer($tblCase)
{
	$iPid = [int]$tblCase.pid
	$sHelperExe = procBuildWinHelper
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	& $sHelperExe signal "$iPid" 2>$null | Out-Null
	if ( $LASTEXITCODE -ne 0 ) {
		throw "signal stop failed: pid=$iPid"
	}

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		$objProc = Get-Process -Id $iPid -ErrorAction SilentlyContinue
		if ( $null -eq $objProc ) {
			procWaitNotReady | Out-Null
			return
		}

		Start-Sleep -Milliseconds 200
	}

	throw "graceful stop timeout: pid=$iPid"
}

function procAssertMemDebugEnabled([string]$sCaseName)
{
	$sStatus = procFetchJsonText "$sManageUrl/__xs/status_json"
	procRequire ($sStatus -like '*"mem_debug":true*') "$sCaseName mem_debug expected true"
}

function procAssertReportNoRegression([string]$sCaseName, [string]$sReportPath, $tblBaseline)
{
	$tblReport = procGetReportSummary $sReportPath

	foreach ( $sKey in $script:arrBaselineKeys ) {
		procRequire (
			([long]$tblReport[$sKey] -le [long]$tblBaseline[$sKey])
		) "$sCaseName report regression: $sKey baseline=$($tblBaseline[$sKey]) actual=$($tblReport[$sKey])"
	}

	procRequire (
		([long]$tblReport.live_allocations_count -eq [long]$tblReport.live_alloc_count)
	) "$sCaseName live_allocations_count mismatch"
	procRequire (
		([long]$tblReport.foreign_live_allocations_count -eq [long]$tblReport.foreign_live_count)
	) "$sCaseName foreign_live_allocations_count mismatch"

	return [ordered]@{
		report = $sReportPath
		live_alloc_count = $tblReport.live_alloc_count
		live_alloc_bytes = $tblReport.live_alloc_bytes
		foreign_live_count = $tblReport.foreign_live_count
		foreign_live_bytes = $tblReport.foreign_live_bytes
		invalid_free_count = $tblReport.invalid_free_count
		double_free_count = $tblReport.double_free_count
		overflow_count = $tblReport.overflow_count
		underflow_count = $tblReport.underflow_count
	}
}

function procWriteSummary([string]$sName, $tblSummary)
{
	Write-Output "[$sName]"

	foreach ( $objEntry in $tblSummary.GetEnumerator() ) {
		Write-Output ("{0}={1}" -f $objEntry.Key, $objEntry.Value)
	}
}

function procBuildCaseSummary($tblBase, $tblExtra)
{
	$tblRet = [ordered]@{}

	foreach ( $objEntry in $tblBase.GetEnumerator() ) {
		$tblRet[$objEntry.Key] = $objEntry.Value
	}
	foreach ( $objEntry in $tblExtra.GetEnumerator() ) {
		$tblRet[$objEntry.Key] = $objEntry.Value
	}

	return $tblRet
}

function procRunHttpCase()
{
	$sRunDir = procPrepareRunDir "http"
	$tblCase = procStartServer "xs_manage_test.json" $sRunDir "http"
	$tblBaseline = procGetCaseBaseline "http"

	try {
		procAssertMemDebugEnabled "http"

		for ( $i = 1; $i -le $iHttpCount; $i++ ) {
			$objResp = procFetchEx "GET" ("{0}/json?run={1}&seq={2}" -f $sManageUrl, $sRunTag, $i)
			procRequire ($objResp.status -eq 200) "http request failed seq=$i status=$($objResp.status)"
			procRequire ($objResp.body -like '*"path":"/json"*') "http body mismatch seq=$i"
		}

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/http_metrics_json"
		procRequire ((procGetMetricInt $tblMetric "http_app_req_count") -eq $iHttpCount) "http_app_req_count mismatch"
	} finally {
		procStopServer $tblCase
	}

	$sReportPath = Join-Path $sRunDir "xrt_mem_report_auto.json"
	procRequire (procWaitFile $sReportPath) "http mem report missing"
	return procBuildCaseSummary ([ordered]@{
		config = $tblCase.config
		requests = $iHttpCount
		log = $tblCase.log_path
	}) (procAssertReportNoRegression "http" $sReportPath $tblBaseline)
}

function procRunWsCase()
{
	$sRunDir = procPrepareRunDir "ws"
	$tblCase = $null
	$tblBaseline = procGetCaseBaseline "ws"

	if ( -not $script:sWsClientExe ) {
		$script:sWsClientExe = procBuildCClient "ws_smoke_client.c" "ws_smoke_client" @("-lws2_32")
	}

	$tblCase = procStartServer "xs_manage_ws_test.json" $sRunDir "ws"

	try {
		procAssertMemDebugEnabled "ws"

		for ( $i = 1; $i -le $iWsCount; $i++ ) {
			$sText = "memdebug-ws-$i"
			$tblRun = procRunClientRetry $script:sWsClientExe @("127.0.0.1", "$iWsPort", $sText)

			procRequire ($tblRun.code -eq 0) "ws client exit=$($tblRun.code) seq=$i"
			procRequire ($tblRun.text -like "*ws demo*") "ws body missing demo seq=$i"
			procRequire ($tblRun.text -like "*text=$sText*") "ws body missing echo seq=$i"
			procRequire ($tblRun.text -like "*protocol=xs-demo*") "ws body missing protocol seq=$i"
		}

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/ws_metrics_json"
		procRequire ((procGetMetricInt $tblMetric "ws_text_count") -ge $iWsCount) "ws_text_count mismatch"
		procRequire ((procGetMetricInt $tblMetric "ws_conn_current") -eq 0) "ws_conn_current mismatch"
	} finally {
		procStopServer $tblCase
	}

	$sReportPath = Join-Path $sRunDir "xrt_mem_report_auto.json"
	procRequire (procWaitFile $sReportPath) "ws mem report missing"
	return procBuildCaseSummary ([ordered]@{
		config = $tblCase.config
		requests = $iWsCount
		log = $tblCase.log_path
	}) (procAssertReportNoRegression "ws" $sReportPath $tblBaseline)
}

function procRunXtpCase()
{
	$sRunDir = procPrepareRunDir "xtp"
	$tblCase = $null
	$tblBaseline = procGetCaseBaseline "xtp"

	if ( -not $script:sXtpClientExe ) {
		$script:sXtpClientExe = procBuildCClient "xtp_pressure_client.c" "xtp_pressure_client" @("-lws2_32")
	}

	$tblCase = procStartServer "xs_manage_xtp_test.json" $sRunDir "xtp"

	try {
		procAssertMemDebugEnabled "xtp"

		$tblRun = procRunClientRetry $script:sXtpClientExe @("127.0.0.1", "$iXtpPort", "$iXtpCount", "demo.ping", "tag=memdebug-xtp-$sRunTag")
		procRequire ($tblRun.code -eq 0) "xtp client exit=$($tblRun.code)"
		procRequire ($tblRun.text -like "*ok_count=$iXtpCount*") "xtp ok_count mismatch"
		procRequire ($tblRun.text -like "*last_status=0*") "xtp last_status mismatch"
		procRequire ($tblRun.text -like "*last_cmd=xtp.reply*") "xtp last_cmd mismatch"

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/xtp_metrics_json"
		procRequire ((procGetMetricInt $tblMetric "xtp_req_count") -ge $iXtpCount) "xtp_req_count mismatch"
		procRequire ((procGetMetricInt $tblMetric "xtp_conn_current") -eq 0) "xtp_conn_current mismatch"
	} finally {
		procStopServer $tblCase
	}

	$sReportPath = Join-Path $sRunDir "xrt_mem_report_auto.json"
	procRequire (procWaitFile $sReportPath) "xtp mem report missing"
	return procBuildCaseSummary ([ordered]@{
		config = $tblCase.config
		requests = $iXtpCount
		log = $tblCase.log_path
	}) (procAssertReportNoRegression "xtp" $sReportPath $tblBaseline)
}

function procRunCustomCase()
{
	$sRunDir = procPrepareRunDir "custom"
	$tblCase = $null
	$tblBaseline = procGetCaseBaseline "custom"

	if ( -not $script:sCustomClientExe ) {
		$script:sCustomClientExe = procBuildCClient "custom_smoke_client.c" "custom_smoke_client" @("-lws2_32")
	}

	$tblCase = procStartServer "xs_manage_custom_test.json" $sRunDir "custom"

	try {
		procAssertMemDebugEnabled "custom"

		for ( $i = 1; $i -le $iCustomCount; $i++ ) {
			$sText = "memdebug-custom-$i"
			$tblRun = procRunClientRetry $script:sCustomClientExe @("127.0.0.1", "$iCustomPort", $sText)

			procRequire ($tblRun.code -eq 0) "custom client exit=$($tblRun.code) seq=$i"
			procRequire ($tblRun.text -like "*custom demo*") "custom body missing demo seq=$i"
			procRequire ($tblRun.text -like "*data=$sText*") "custom body missing echo seq=$i"
		}

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/custom_metrics_json"
		procRequire ((procGetMetricInt $tblMetric "custom_recv_count") -ge $iCustomCount) "custom_recv_count mismatch"
		procRequire ((procGetMetricInt $tblMetric "custom_conn_current") -eq 0) "custom_conn_current mismatch"
	} finally {
		procStopServer $tblCase
	}

	$sReportPath = Join-Path $sRunDir "xrt_mem_report_auto.json"
	procRequire (procWaitFile $sReportPath) "custom mem report missing"
	return procBuildCaseSummary ([ordered]@{
		config = $tblCase.config
		requests = $iCustomCount
		log = $tblCase.log_path
	}) (procAssertReportNoRegression "custom" $sReportPath $tblBaseline)
}

function procCheckCleanup([string]$sRootHashBefore, [string]$sReleaseHashBefore)
{
	$arrProc = @(Get-Process xs, xsdbg -ErrorAction SilentlyContinue)
	$arrHelper = @()

	foreach ( $sPattern in @(
		"ws_smoke_client.memdebug.*.exe",
		"xtp_pressure_client.memdebug.*.exe",
		"custom_smoke_client.memdebug.*.exe",
		"win_console_group_helper.memdebug.*.exe"
	) ) {
		$arrHelper += @(Get-ChildItem -Path $sToolDir -Filter $sPattern -File -ErrorAction SilentlyContinue)
	}

	procRequire ($arrProc.Count -eq 0) "cleanup_processes remained"
	procRequire ($arrHelper.Count -eq 0) "generated helpers remained"
	procRequire ((procGetFileHashText (Join-Path $sRepoRoot "xrt_mem_report_auto.json")) -eq $sRootHashBefore) "tracked root mem report changed"
	procRequire ((procGetFileHashText (Join-Path $sReleaseDir "xrt_mem_report_auto.json")) -eq $sReleaseHashBefore) "tracked release mem report changed"
}

try {
	$sRootHashBefore = procGetFileHashText (Join-Path $sRepoRoot "xrt_mem_report_auto.json")
	$sReleaseHashBefore = procGetFileHashText (Join-Path $sReleaseDir "xrt_mem_report_auto.json")

	Remove-Item $sTempRoot -Recurse -Force -ErrorAction SilentlyContinue
	New-Item -ItemType Directory -Path $sTempRoot | Out-Null

	procCleanupGeneratedFiles
	procCleanupServers

	procWriteSummary "http" (procRunHttpCase)
	procWriteSummary "ws" (procRunWsCase)
	procWriteSummary "xtp" (procRunXtpCase)
	procWriteSummary "custom" (procRunCustomCase)

	procCleanupServers
	procCleanupGeneratedFiles
	procCheckCleanup $sRootHashBefore $sReleaseHashBefore

	Write-Output "[status]"
	Write-Output "result=ok"
} catch {
	$script:bKeepTemp = $true
	throw
} finally {
	procCleanupServers
	procCleanupGeneratedFiles

	if ( (Test-Path $sTempRoot) -and (-not $script:bKeepTemp) ) {
		Remove-Item $sTempRoot -Recurse -Force -ErrorAction SilentlyContinue
	}
}
