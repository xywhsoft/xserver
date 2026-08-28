param(
	[int]$iHttpCount = 50,
	[int]$iWsCount = 20,
	[int]$iXtpCount = 20,
	[int]$iCustomCount = 20,
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
$script:sWsClientExe = $null
$script:sXtpClientExe = $null
$script:sCustomClientExe = $null

function procCleanupGeneratedFiles()
{
	foreach ( $sPattern in @(
		"ws_smoke_client.pressure.*.exe",
		"xtp_smoke_client.pressure.*.exe",
		"xtp_pressure_client.pressure.*.exe",
		"custom_smoke_client.pressure.*.exe"
	) ) {
		Get-ChildItem -Path $sToolDir -Filter $sPattern -File -ErrorAction SilentlyContinue | ForEach-Object {
			Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
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

function procCleanupServers()
{
	Get-Process xs, xsdbg -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
	procWaitNotReady | Out-Null
}

function procStartServer([string]$sConfig)
{
	$sExePath = Join-Path $sReleaseDir "xsdbg.exe"

	if ( -not (Test-Path $sExePath) ) {
		throw "xsdbg.exe not found"
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList $sConfig -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden
	if ( -not (procWaitReady) ) {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		throw "server ready timeout: $sConfig"
	}

	return $objProc
}

function procStopServer($objProc)
{
	if ( $objProc -and (-not $objProc.HasExited) ) {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
	}

	Start-Sleep -Milliseconds 500
	procWaitNotReady | Out-Null
}

function procBuildCClient([string]$sSourceName, [string]$sOutputBase)
{
	$sSource = Join-Path $sToolDir $sSourceName
	$sOutput = Join-Path $sToolDir ("{0}.pressure.{1}.exe" -f $sOutputBase, $sRunTag)
	$arrArgs = @($sSource, "-O2", "-s", "-lws2_32", "-o", $sOutput)

	if ( -not (Test-Path $sSource) ) {
		throw "$sSourceName not found"
	}

	& gcc @arrArgs | Out-Null
	if ( ($LASTEXITCODE -ne 0) -or (-not (Test-Path $sOutput)) ) {
		throw "build $sSourceName failed"
	}

	return $sOutput
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

function procCalcAvgMS([int]$iCount, [long]$iElapsedMS)
{
	if ( $iCount -le 0 ) {
		return "0.00"
	}

	return [string]([Math]::Round(($iElapsedMS * 1.0) / $iCount, 2))
}

function procCalcRPS([int]$iCount, [long]$iElapsedMS)
{
	if ( $iElapsedMS -le 0 ) {
		return "0.00"
	}

	return [string]([Math]::Round(($iCount * 1000.0) / $iElapsedMS, 2))
}

function procWriteSummary([string]$sName, $tblSummary)
{
	Write-Output "[$sName]"

	foreach ( $objEntry in $tblSummary.GetEnumerator() ) {
		Write-Output ("{0}={1}" -f $objEntry.Key, $objEntry.Value)
	}
}

function procRunHttpBaseline()
{
	if ( $iHttpCount -le 0 ) {
		return [ordered]@{
			config = "xs_manage_test.json"
			skipped = "true"
			requests = 0
		}
	}

	$objProc = procStartServer "xs_manage_test.json"

	try {
		procRequireStatus (procFetchEx "GET" "$sManageUrl/__xs/http_metrics_clear") 200 "http_metrics_clear"

		$objWatch = [System.Diagnostics.Stopwatch]::StartNew()
		$iSuccess = 0

		for ( $i = 1; $i -le $iHttpCount; $i++ ) {
			$objResp = procFetchEx "GET" ("{0}/json?run={1}&seq={2}" -f $sManageUrl, $sRunTag, $i)
			procRequire ($objResp.status -eq 200) "http request failed at seq=$i status=$($objResp.status)"
			procRequire ($objResp.body -like '*"path":"/json"*') "http body mismatch at seq=$i"
			$iSuccess++
		}

		$objWatch.Stop()
		Start-Sleep -Milliseconds 200

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/http_metrics_json"
		$iAppCount = procGetMetricInt $tblMetric "http_app_req_count"
		procRequire ($iAppCount -eq $iHttpCount) "http_app_req_count expected $iHttpCount, got $iAppCount"

		return [ordered]@{
			config = "xs_manage_test.json"
			requests = $iHttpCount
			success = $iSuccess
			failures = ($iHttpCount - $iSuccess)
			elapsed_ms = $objWatch.ElapsedMilliseconds
			avg_ms = (procCalcAvgMS $iSuccess $objWatch.ElapsedMilliseconds)
			rps = (procCalcRPS $iSuccess $objWatch.ElapsedMilliseconds)
			server_http_app_req_count = $iAppCount
			server_http_manage_req_count = (procGetMetricInt $tblMetric "http_manage_req_count")
			server_http_time_avg_ms = (procGetMetricInt $tblMetric "http_time_avg_ms")
			server_http_last_app_path = (procGetMetricText $tblMetric "http_last_app_path")
		}
	} finally {
		procStopServer $objProc
	}
}

function procRunWsBaseline()
{
	if ( $iWsCount -le 0 ) {
		return [ordered]@{
			config = "xs_manage_ws_test.json"
			skipped = "true"
			requests = 0
		}
	}

	if ( -not $script:sWsClientExe ) {
		$script:sWsClientExe = procBuildCClient "ws_smoke_client.c" "ws_smoke_client"
	}

	$objProc = procStartServer "xs_manage_ws_test.json"

	try {
		procRequireStatus (procFetchEx "GET" "$sManageUrl/__xs/ws_metrics_clear") 200 "ws_metrics_clear"
		Start-Sleep -Milliseconds 500

		$objWatch = [System.Diagnostics.Stopwatch]::StartNew()
		$iSuccess = 0

		for ( $i = 1; $i -le $iWsCount; $i++ ) {
			$sText = "baseline-ws-$i"
			$tblRun = procRunClientRetry $script:sWsClientExe @("127.0.0.1", "$iWsPort", $sText)

			procRequire ($tblRun.code -eq 0) "ws client exit=$($tblRun.code) seq=$i text=$($tblRun.text)"
			procRequire ($tblRun.text -like "*ws demo*") "ws body missing demo seq=$i"
			procRequire ($tblRun.text -like "*text=$sText*") "ws body missing echo seq=$i"
			procRequire ($tblRun.text -like "*protocol=xs-demo*") "ws body missing protocol seq=$i"
			$iSuccess++
		}

		$objWatch.Stop()
		Start-Sleep -Milliseconds 300

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/ws_metrics_json"
		$iOpenCount = procGetMetricInt $tblMetric "ws_open_count"
		$iCloseCount = procGetMetricInt $tblMetric "ws_close_count"
		$iTextCount = procGetMetricInt $tblMetric "ws_text_count"
		$iConnCurrent = procGetMetricInt $tblMetric "ws_conn_current"

		procRequire ($iOpenCount -ge $iWsCount) "ws_open_count expected >= $iWsCount, got $iOpenCount"
		procRequire ($iCloseCount -ge $iWsCount) "ws_close_count expected >= $iWsCount, got $iCloseCount"
		procRequire ($iTextCount -ge $iWsCount) "ws_text_count expected >= $iWsCount, got $iTextCount"
		procRequire ($iConnCurrent -eq 0) "ws_conn_current expected 0, got $iConnCurrent"

		return [ordered]@{
			config = "xs_manage_ws_test.json"
			requests = $iWsCount
			success = $iSuccess
			failures = ($iWsCount - $iSuccess)
			elapsed_ms = $objWatch.ElapsedMilliseconds
			avg_ms = (procCalcAvgMS $iSuccess $objWatch.ElapsedMilliseconds)
			rps = (procCalcRPS $iSuccess $objWatch.ElapsedMilliseconds)
			server_ws_open_count = $iOpenCount
			server_ws_close_count = $iCloseCount
			server_ws_text_count = $iTextCount
			server_ws_last_text = (procGetMetricText $tblMetric "ws_last_text")
		}
	} finally {
		procStopServer $objProc
	}
}

function procRunXtpBaseline()
{
	if ( $iXtpCount -le 0 ) {
		return [ordered]@{
			config = "xs_manage_xtp_test.json"
			skipped = "true"
			requests = 0
		}
	}

	if ( -not $script:sXtpClientExe ) {
		$script:sXtpClientExe = procBuildCClient "xtp_pressure_client.c" "xtp_pressure_client"
	}

	$objProc = procStartServer "xs_manage_xtp_test.json"

	try {
		procRequireStatus (procFetchEx "GET" "$sManageUrl/__xs/xtp_metrics_clear") 200 "xtp_metrics_clear"
		Start-Sleep -Milliseconds 500

		$objWatch = [System.Diagnostics.Stopwatch]::StartNew()
		$tblRun = procRunClientRetry $script:sXtpClientExe @("127.0.0.1", "$iXtpPort", "$iXtpCount", "demo.ping", "tag=baseline-xtp-$sRunTag")
		$objWatch.Stop()

		procRequire ($tblRun.code -eq 0) "xtp client exit=$($tblRun.code) text=$($tblRun.text)"
		procRequire ($tblRun.text -like "*ok_count=$iXtpCount*") "xtp ok_count mismatch"
		procRequire ($tblRun.text -like "*last_status=0*") "xtp last_status mismatch"
		procRequire ($tblRun.text -like "*last_cmd=xtp.reply*") "xtp last_cmd mismatch"

		$iSuccess = $iXtpCount
		Start-Sleep -Milliseconds 300

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/xtp_metrics_json"
		$iOpenCount = procGetMetricInt $tblMetric "xtp_open_count"
		$iCloseCount = procGetMetricInt $tblMetric "xtp_close_count"
		$iMsgCount = procGetMetricInt $tblMetric "xtp_msg_count"
		$iReqCount = procGetMetricInt $tblMetric "xtp_req_count"
		$iSendCount = procGetMetricInt $tblMetric "xtp_send_count"
		$iConnCurrent = procGetMetricInt $tblMetric "xtp_conn_current"

		procRequire ($iOpenCount -ge 1) "xtp_open_count expected >= 1, got $iOpenCount"
		procRequire ($iCloseCount -ge 1) "xtp_close_count expected >= 1, got $iCloseCount"
		procRequire ($iMsgCount -ge $iXtpCount) "xtp_msg_count expected >= $iXtpCount, got $iMsgCount"
		procRequire ($iReqCount -ge $iXtpCount) "xtp_req_count expected >= $iXtpCount, got $iReqCount"
		procRequire ($iSendCount -ge $iXtpCount) "xtp_send_count expected >= $iXtpCount, got $iSendCount"
		procRequire ($iConnCurrent -eq 0) "xtp_conn_current expected 0, got $iConnCurrent"

		return [ordered]@{
			config = "xs_manage_xtp_test.json"
			requests = $iXtpCount
			success = $iSuccess
			failures = ($iXtpCount - $iSuccess)
			elapsed_ms = $objWatch.ElapsedMilliseconds
			avg_ms = (procCalcAvgMS $iSuccess $objWatch.ElapsedMilliseconds)
			rps = (procCalcRPS $iSuccess $objWatch.ElapsedMilliseconds)
			server_xtp_open_count = $iOpenCount
			server_xtp_close_count = $iCloseCount
			server_xtp_msg_count = $iMsgCount
			server_xtp_req_count = $iReqCount
			server_xtp_send_count = $iSendCount
			server_xtp_last_cmd = (procGetMetricText $tblMetric "xtp_last_cmd")
		}
	} finally {
		procStopServer $objProc
	}
}

function procRunCustomBaseline()
{
	if ( $iCustomCount -le 0 ) {
		return [ordered]@{
			config = "xs_manage_custom_test.json"
			skipped = "true"
			requests = 0
		}
	}

	if ( -not $script:sCustomClientExe ) {
		$script:sCustomClientExe = procBuildCClient "custom_smoke_client.c" "custom_smoke_client"
	}

	$objProc = procStartServer "xs_manage_custom_test.json"

	try {
		procRequireStatus (procFetchEx "GET" "$sManageUrl/__xs/custom_metrics_clear") 200 "custom_metrics_clear"
		Start-Sleep -Milliseconds 500

		$objWatch = [System.Diagnostics.Stopwatch]::StartNew()
		$iSuccess = 0

		for ( $i = 1; $i -le $iCustomCount; $i++ ) {
			$sText = "baseline-custom-$i"
			$tblRun = procRunClientRetry $script:sCustomClientExe @("127.0.0.1", "$iCustomPort", $sText)

			procRequire ($tblRun.code -eq 0) "custom client exit=$($tblRun.code) seq=$i text=$($tblRun.text)"
			procRequire ($tblRun.text -like "*custom demo*") "custom body missing demo seq=$i"
			procRequire ($tblRun.text -like "*data=$sText*") "custom body missing echo seq=$i"
			$iSuccess++
		}

		$objWatch.Stop()
		Start-Sleep -Milliseconds 300

		$tblMetric = procFetchJsonText "$sManageUrl/__xs/custom_metrics_json"
		$iOpenCount = procGetMetricInt $tblMetric "custom_open_count"
		$iCloseCount = procGetMetricInt $tblMetric "custom_close_count"
		$iRecvCount = procGetMetricInt $tblMetric "custom_recv_count"
		$iSendCount = procGetMetricInt $tblMetric "custom_send_count"
		$iConnCurrent = procGetMetricInt $tblMetric "custom_conn_current"

		procRequire ($iOpenCount -ge $iCustomCount) "custom_open_count expected >= $iCustomCount, got $iOpenCount"
		procRequire ($iCloseCount -ge $iCustomCount) "custom_close_count expected >= $iCustomCount, got $iCloseCount"
		procRequire ($iRecvCount -ge $iCustomCount) "custom_recv_count expected >= $iCustomCount, got $iRecvCount"
		procRequire ($iSendCount -ge $iCustomCount) "custom_send_count expected >= $iCustomCount, got $iSendCount"
		procRequire ($iConnCurrent -eq 0) "custom_conn_current expected 0, got $iConnCurrent"

		return [ordered]@{
			config = "xs_manage_custom_test.json"
			requests = $iCustomCount
			success = $iSuccess
			failures = ($iCustomCount - $iSuccess)
			elapsed_ms = $objWatch.ElapsedMilliseconds
			avg_ms = (procCalcAvgMS $iSuccess $objWatch.ElapsedMilliseconds)
			rps = (procCalcRPS $iSuccess $objWatch.ElapsedMilliseconds)
			server_custom_open_count = $iOpenCount
			server_custom_close_count = $iCloseCount
			server_custom_recv_count = $iRecvCount
			server_custom_send_count = $iSendCount
			server_custom_last_text = (procGetMetricText $tblMetric "custom_last_text")
		}
	} finally {
		procStopServer $objProc
	}
}

$iExit = 0

procCleanupGeneratedFiles
procCleanupServers

try {
	procWriteSummary "http" (procRunHttpBaseline)
	procWriteSummary "ws" (procRunWsBaseline)
	procWriteSummary "xtp" (procRunXtpBaseline)
	procWriteSummary "custom" (procRunCustomBaseline)
	Write-Output "[result]"
	Write-Output "status=ok"
} catch {
	$iExit = 1
	Write-Output "[result]"
	Write-Output "status=fail"
	Write-Output ("message={0}" -f $_.Exception.Message)
} finally {
	foreach ( $sPath in @($script:sWsClientExe, $script:sXtpClientExe, $script:sCustomClientExe) ) {
		if ( $sPath -and (Test-Path $sPath) ) {
			Remove-Item $sPath -Force -ErrorAction SilentlyContinue
		}
	}

	procCleanupGeneratedFiles
	procCleanupServers
}

exit $iExit
