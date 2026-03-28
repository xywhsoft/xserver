param(
	[string]$sConfig = "xs_manage_test.json"
)

$iPort = 8085
$iXtpPort = 9096
$iWaitMS = 10000
$sRepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$sReleaseDir = Join-Path $sRepoRoot "release"
$sToolDir = Join-Path $sRepoRoot "tools"
$sIndexFile = Join-Path $sReleaseDir "wwwroot\index.html"
$script:sRootMemReport = Join-Path $sRepoRoot "xrt_mem_report_auto.json"
$script:sReleaseMemReport = Join-Path $sReleaseDir "xrt_mem_report_auto.json"
$sRunTag = [string]$PID
$script:sXtpClientExe = $null
$script:sWsClientExe = $null
$script:sCustomClientExe = $null

function procCleanupGeneratedFiles()
{
	foreach ( $sPattern in @(
		"xtp_smoke_client*.exe",
		"ws_smoke_client*.exe",
		"custom_smoke_client*.exe",
		".xs_stable_smoke_body.tmp"
	) ) {
		Get-ChildItem -Path $sToolDir -Filter $sPattern -File -ErrorAction SilentlyContinue | ForEach-Object {
			Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
		}
	}
}

function procGetFileHashText([string]$sPath)
{
	if ( -not (Test-Path $sPath) ) {
		return "(missing)"
	}

	return (Get-FileHash -Algorithm SHA256 $sPath).Hash
}

function procAppendArtifactCleanupCheck(
	[System.Collections.Generic.List[string]]$arrOut,
	[string]$sRootHashBefore,
	[string]$sReleaseHashBefore
)
{
	$arrOut.Add("[artifacts]") | Out-Null
	$iExit = 0

	$sRootHashAfter = procGetFileHashText $script:sRootMemReport
	if ( $sRootHashAfter -eq $sRootHashBefore ) {
		$arrOut.Add("OK   tracked_mem_report_root : unchanged") | Out-Null
	} else {
		$arrOut.Add("FAIL tracked_mem_report_root : changed") | Out-Null
		$iExit = 1
	}

	$sReleaseHashAfter = procGetFileHashText $script:sReleaseMemReport
	if ( $sReleaseHashAfter -eq $sReleaseHashBefore ) {
		$arrOut.Add("OK   tracked_mem_report_release : unchanged") | Out-Null
	} else {
		$arrOut.Add("FAIL tracked_mem_report_release : changed") | Out-Null
		$iExit = 1
	}

	$arrHelper = @()
	foreach ( $sPattern in @(
		"xtp_smoke_client*.exe",
		"ws_smoke_client*.exe",
		"custom_smoke_client*.exe",
		".xs_stable_smoke_body.tmp"
	) ) {
		$arrHelper += @(Get-ChildItem -Path $sToolDir -Filter $sPattern -File -ErrorAction SilentlyContinue)
	}

	if ( $arrHelper.Count -le 0 ) {
		$arrOut.Add("OK   generated_helpers : none") | Out-Null
	} else {
		foreach ( $objFile in $arrHelper ) {
			$arrOut.Add(("FAIL generated_helpers : {0}" -f $objFile.Name)) | Out-Null
		}
		$iExit = 1
	}

	return $iExit
}

function procAppendProcessCleanupCheck([System.Collections.Generic.List[string]]$arrOut)
{
	$arrOut.Add("[cleanup]") | Out-Null

	$arrProc = @(Get-Process xs, xsdbg -ErrorAction SilentlyContinue)
	if ( $arrProc.Count -le 0 ) {
		$arrOut.Add("OK   cleanup_processes : none") | Out-Null
		return 0
	}

	foreach ( $objProc in $arrProc ) {
		$arrOut.Add(("FAIL cleanup_processes : {0} pid={1}" -f $objProc.ProcessName, $objProc.Id)) | Out-Null
	}

	return 1
}

function procFetch([string]$sUrl)
{
	return procFetchEx "GET" $sUrl
}

function procFetchEx([string]$sMethod, [string]$sUrl)
{
	try {
		$objResp = Invoke-WebRequest -UseBasicParsing $sUrl -Method $sMethod -TimeoutSec 2 -ErrorAction Stop
		return @{
			status = [int]$objResp.StatusCode
			body = [string]$objResp.Content
		}
	} catch {
		if ( $_.Exception.Response ) {
			$objStream = $_.Exception.Response.GetResponseStream()
			$objReader = New-Object System.IO.StreamReader($objStream)
			$sBody = $objReader.ReadToEnd()
			$objReader.Dispose()
			return @{
				status = [int]$_.Exception.Response.StatusCode
				body = $sBody
			}
		}
		throw
	}
}

function procFetchStatusMethod([string]$sMethod, [string]$sUrl)
{
	$arrArgs = @("-s", "-o", "NUL", "-w", "%{http_code}")

	if ( $sMethod -eq "HEAD" ) {
		$arrArgs += "-I"
	} else {
		$arrArgs += @("-X", $sMethod)
	}

	$arrArgs += $sUrl
	$sStatus = (& curl.exe @arrArgs 2>$null).Trim()
	return [int]$sStatus
}

function procFetchAllowMethod([string]$sMethod, [string]$sUrl)
{
	return procFetchHeaderMethod $sMethod $sUrl "Allow"
}

function procFetchHeaderMethod([string]$sMethod, [string]$sUrl, [string]$sHeaderName)
{
	$arrArgs = @("-s", "-D", "-", "-o", "NUL")

	if ( $sMethod -eq "HEAD" ) {
		$arrArgs += "-I"
	} else {
		$arrArgs += @("-X", $sMethod)
	}

	$arrArgs += $sUrl
	$sHeaderText = (& curl.exe @arrArgs 2>$null) -join "`n"
	$arrLines = $sHeaderText -split "`r?`n"

	foreach ( $sLine in $arrLines ) {
		if ( $sLine -match ('^(?i)' + [Regex]::Escape($sHeaderName) + ':\s*(.+)$') ) {
			return $Matches[1].Trim()
		}
	}

	return ""
}

function procFetchBodyMethod([string]$sMethod, [string]$sUrl)
{
	$arrArgs = @("-s")

	if ( $sMethod -ne "GET" ) {
		$arrArgs += @("-X", $sMethod)
	}

	$arrArgs += $sUrl
	return ((& curl.exe @arrArgs 2>$null) -join "`n")
}

function procCheckBodyContains([System.Collections.Generic.List[string]]$arrResults, [string]$sName, [string]$sBody, [string]$sExpectText)
{
	if ( $sBody -like "*$sExpectText*" ) {
		$arrResults.Add("OK   $sName : body")
		return
	}

	$arrResults.Add("FAIL $sName : missing text '$sExpectText'")
}

function procCheckSecurityHeaders([System.Collections.Generic.List[string]]$arrResults, [string]$sName, [string]$sMethod, [string]$sUrl)
{
	procCheckBodyContains $arrResults "$sName cache" (procFetchHeaderMethod $sMethod $sUrl "Cache-Control") 'no-store'
	procCheckBodyContains $arrResults "$sName frame" (procFetchHeaderMethod $sMethod $sUrl "X-Frame-Options") 'DENY'
	procCheckBodyContains $arrResults "$sName referrer" (procFetchHeaderMethod $sMethod $sUrl "Referrer-Policy") 'no-referrer'
	procCheckBodyContains $arrResults "$sName nosniff" (procFetchHeaderMethod $sMethod $sUrl "X-Content-Type-Options") 'nosniff'
}

function procCheckDisabledEndpoint([System.Collections.Generic.List[string]]$arrResults, [string]$sExeName, [string]$sName, [string]$sMethod, [string]$sPath, [int]$iExpectStatus, [string]$sContentType, [string]$sBodyText)
{
	$sUrl = "http://127.0.0.1:$iPort$sPath"
	$iStatus = procFetchStatusMethod $sMethod $sUrl

	procCheck $arrResults "$sExeName $sName" $iStatus $iExpectStatus ""
	if ( -not [string]::IsNullOrEmpty($sBodyText) ) {
		procCheckBodyContains $arrResults "$sExeName $sName body" (procFetchBodyMethod $sMethod $sUrl) $sBodyText
	}
	procCheckBodyContains $arrResults "$sExeName $sName type" (procFetchHeaderMethod $sMethod $sUrl "Content-Type") $sContentType
	procCheckSecurityHeaders $arrResults "$sExeName $sName" $sMethod $sUrl
}

function procCheckMethodReject([System.Collections.Generic.List[string]]$arrResults, [string]$sExeName, [string]$sName, [string]$sMethod, [string]$sPath, [int]$iExpectStatus, [string]$sAllow, [string]$sContentType)
{
	$sUrl = "http://127.0.0.1:$iPort$sPath"
	$iStatus = procFetchStatusMethod $sMethod $sUrl

	procCheck $arrResults "$sExeName $sName" $iStatus $iExpectStatus ""
	procCheckBodyContains $arrResults "$sExeName $sName allow" (procFetchAllowMethod $sMethod $sUrl) $sAllow
	procCheckBodyContains $arrResults "$sExeName $sName type" (procFetchHeaderMethod $sMethod $sUrl "Content-Type") $sContentType
	procCheckSecurityHeaders $arrResults "$sExeName $sName" $sMethod $sUrl
}

function procCheckLineWithMarker([System.Collections.Generic.List[string]]$arrResults, [string[]]$arrLines, [string]$sName, [string]$sPathText, [string]$sMarkerText)
{
	$i;

	for ( $i = 0; $i -lt $arrLines.Length; $i++ ) {
		if ( ($arrLines[$i] -like "*$sPathText*") -and ($arrLines[$i] -like "*$sMarkerText*") ) {
			$arrResults.Add("OK   $sName : static")
			return
		}
	}

	$arrResults.Add("FAIL $sName : missing '$sMarkerText' on '$sPathText'")
}

function procWaitReady()
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		try {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/status_json"
			if ( $objResp.status -eq 200 ) {
				return $true
			}
		} catch {
		}

		Start-Sleep -Milliseconds 200
	}

	return $false
}

function procWaitReloadIdle()
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		try {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_status_json"
			if ( ($objResp.status -eq 200) -and ($objResp.body -notlike '*"busy":true*') ) {
				return $true
			}
		} catch {
		}

		Start-Sleep -Milliseconds 200
	}

	return $false
}

function procWaitBodyContains([string]$sUrl, [string]$sExpectText)
{
	$tEnd = [DateTime]::UtcNow.AddMilliseconds($iWaitMS)

	while ( [DateTime]::UtcNow -lt $tEnd ) {
		try {
			$objResp = procFetch $sUrl
			if ( ($objResp.status -eq 200) -and ($objResp.body -like "*$sExpectText*") ) {
				return $objResp
			}
		} catch {
		}

		Start-Sleep -Milliseconds 200
	}

	return $null
}

function procBuildXtpSmokeClient()
{
	if ( $script:sXtpClientExe -and (Test-Path $script:sXtpClientExe) ) {
		return $script:sXtpClientExe
	}

	$script:sXtpClientExe = procBuildCClient "xtp_smoke_client.c" "xtp_smoke_client"
	return $script:sXtpClientExe
}

function procBuildCClient([string]$sSourceName, [string]$sOutputName)
{
	$sSource = Join-Path $sToolDir $sSourceName
	$sOutput = Join-Path $sToolDir ([System.IO.Path]::GetFileNameWithoutExtension($sOutputName) + "." + $sRunTag + ".exe")
	$arrArgs = @($sSource, "-O2", "-s", "-lws2_32", "-o", $sOutput)

	if ( -not (Test-Path $sSource) ) {
		throw "$sSourceName not found"
	}

	& gcc @arrArgs | Out-Null
	if ( $LASTEXITCODE -ne 0 -or -not (Test-Path $sOutput) ) {
		throw "build $sSourceName failed"
	}

	return $sOutput
}

function procRunXtpHelper([string]$sClientExe, [string[]]$arrArgs)
{
	$sOutput = & $sClientExe @arrArgs 2>&1
	$iCode = $LASTEXITCODE

	return @{
		code = $iCode
		text = (($sOutput | ForEach-Object { $_.ToString() }) -join "`n")
	}
}

function procRunClientRetry([string]$sClientExe, [string[]]$arrArgs, [int]$iRetryCount = 10)
{
	$tblRun = $null

	for ( $i = 0; $i -lt $iRetryCount; $i++ ) {
		$tblRun = procRunXtpHelper $sClientExe $arrArgs
		if ( $tblRun.code -eq 0 ) {
			return $tblRun
		}

		if ( $i -lt ($iRetryCount - 1) ) {
			Start-Sleep -Milliseconds 200
		}
	}

	return $tblRun
}

function procCheck([System.Collections.Generic.List[string]]$arrResults, [string]$sName, [int]$iActual, [int]$iExpect, [string]$sBody, [string]$sExpectText = $null)
{
	if ( $iActual -ne $iExpect ) {
		$arrResults.Add("FAIL $sName : expected $iExpect, got $iActual")
		return
	}

	if ( $sExpectText -and ($sBody -notlike "*$sExpectText*") ) {
		$arrResults.Add("FAIL $sName : missing text '$sExpectText'")
		return
	}

	$arrResults.Add("OK   $sName : $iActual")
}

function procAppendLines([System.Collections.Generic.List[string]]$arrTarget, $arrLines)
{
	$item;

	foreach ( $item in $arrLines ) {
		$arrTarget.Add([string]$item)
	}
}

function procRunStaticHomepageAudit()
{
	$arrResults = New-Object 'System.Collections.Generic.List[string]'
	$arrAudit = @(
		@{ name = 'homepage dashboard link'; path = '/__xs/dashboard'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage dashboard_json link'; path = '/__xs/dashboard_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage http_metrics_json link'; path = '/__xs/http_metrics_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage http_metrics_clear link'; path = '/__xs/http_metrics_clear'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage ws_metrics_json link'; path = '/__xs/ws_metrics_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage ws_metrics_clear link'; path = '/__xs/ws_metrics_clear'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage xtp_metrics_json link'; path = '/__xs/xtp_metrics_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage xtp_metrics_clear link'; path = '/__xs/xtp_metrics_clear'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage udp_metrics_json link'; path = '/__xs/udp_metrics_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage udp_metrics_clear link'; path = '/__xs/udp_metrics_clear'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage custom_metrics_json link'; path = '/__xs/custom_metrics_json'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage custom_metrics_clear link'; path = '/__xs/custom_metrics_clear'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage bus_status link'; path = '/__xs/bus/status'; marker = 'data-bus-manage="true"' },
		@{ name = 'homepage reload_reset link'; path = '/__xs/reload_reset'; marker = 'data-debug-manage="true"' },
		@{ name = 'homepage check_config_clear link'; path = '/__xs/check_config_clear'; marker = 'data-debug-manage="true"' }
	)
	$arrLines = @()
	$i;

	if ( -not (Test-Path $sIndexFile) ) {
		$arrResults.Add("FAIL static homepage audit : index.html not found")
		return @{
			code = 1
			lines = $arrResults
		}
	}

	$arrLines = Get-Content $sIndexFile

	for ( $i = 0; $i -lt $arrAudit.Length; $i++ ) {
		procCheckLineWithMarker $arrResults $arrLines $arrAudit[$i].name $arrAudit[$i].path $arrAudit[$i].marker
	}

	if ( $arrResults -match '^FAIL ' ) {
		return @{
			code = 1
			lines = $arrResults
		}
	}

	return @{
		code = 0
		lines = $arrResults
	}
}

function procRunCase([string]$sExeName, [bool]$bDebug)
{
	$sExePath = Join-Path $sReleaseDir $sExeName
	$arrResults = New-Object 'System.Collections.Generic.List[string]'
	$arrMetricsClear = @(
		@{ name = 'http_metrics_clear'; path = '/__xs/http_metrics_clear'; prod = 'http metrics clear api only available in xsdbg' },
		@{ name = 'ws_metrics_clear'; path = '/__xs/ws_metrics_clear'; prod = 'ws metrics clear api only available in xsdbg' },
		@{ name = 'xtp_metrics_clear'; path = '/__xs/xtp_metrics_clear'; prod = 'xtp metrics clear api only available in xsdbg' },
		@{ name = 'udp_metrics_clear'; path = '/__xs/udp_metrics_clear'; prod = 'udp metrics clear api only available in xsdbg' },
		@{ name = 'custom_metrics_clear'; path = '/__xs/custom_metrics_clear'; prod = 'custom metrics clear api only available in xsdbg' }
	)
	$arrMetricsText = @(
		@{ name = 'http_metrics'; path = '/__xs/http_metrics'; token = 'http_req_count=' },
		@{ name = 'ws_metrics'; path = '/__xs/ws_metrics'; token = 'ws_open_count=' },
		@{ name = 'xtp_metrics'; path = '/__xs/xtp_metrics'; token = 'xtp_open_count=' },
		@{ name = 'udp_metrics'; path = '/__xs/udp_metrics'; token = 'udp_recv_count=' },
		@{ name = 'custom_metrics'; path = '/__xs/custom_metrics'; token = 'custom_open_count=' }
	)
	$arrMetricsJson = @(
		@{ name = 'http_metrics_json'; path = '/__xs/http_metrics_json'; token = '"http_req_count"' },
		@{ name = 'ws_metrics_json'; path = '/__xs/ws_metrics_json'; token = '"ws_open_count"' },
		@{ name = 'xtp_metrics_json'; path = '/__xs/xtp_metrics_json'; token = '"xtp_open_count"' },
		@{ name = 'udp_metrics_json'; path = '/__xs/udp_metrics_json'; token = '"udp_recv_count"' },
		@{ name = 'custom_metrics_json'; path = '/__xs/custom_metrics_json'; token = '"custom_open_count"' }
	)
	$arrMetricsClearText = @(
		@{ name = 'http_metrics_clear'; path = '/__xs/http_metrics_clear'; token = 'http_req_count=' },
		@{ name = 'ws_metrics_clear'; path = '/__xs/ws_metrics_clear'; token = 'ws_open_count=' },
		@{ name = 'xtp_metrics_clear'; path = '/__xs/xtp_metrics_clear'; token = 'xtp_open_count=' },
		@{ name = 'udp_metrics_clear'; path = '/__xs/udp_metrics_clear'; token = 'udp_recv_count=' },
		@{ name = 'custom_metrics_clear'; path = '/__xs/custom_metrics_clear'; token = 'custom_open_count=' }
	)
	$arrDebugDisabledText = @(
		@{ name = 'dashboard'; path = '/__xs/dashboard'; prod = 'dashboard api only available in xsdbg' },
		@{ name = 'http_metrics'; path = '/__xs/http_metrics'; prod = 'http metrics api only available in xsdbg' },
		@{ name = 'ws_metrics'; path = '/__xs/ws_metrics'; prod = 'ws metrics api only available in xsdbg' },
		@{ name = 'xtp_metrics'; path = '/__xs/xtp_metrics'; prod = 'xtp metrics api only available in xsdbg' },
		@{ name = 'udp_metrics'; path = '/__xs/udp_metrics'; prod = 'udp metrics api only available in xsdbg' },
		@{ name = 'custom_metrics'; path = '/__xs/custom_metrics'; prod = 'custom metrics api only available in xsdbg' },
		@{ name = 'http_metrics_clear'; path = '/__xs/http_metrics_clear'; prod = 'http metrics clear api only available in xsdbg' },
		@{ name = 'ws_metrics_clear'; path = '/__xs/ws_metrics_clear'; prod = 'ws metrics clear api only available in xsdbg' },
		@{ name = 'xtp_metrics_clear'; path = '/__xs/xtp_metrics_clear'; prod = 'xtp metrics clear api only available in xsdbg' },
		@{ name = 'udp_metrics_clear'; path = '/__xs/udp_metrics_clear'; prod = 'udp metrics clear api only available in xsdbg' },
		@{ name = 'custom_metrics_clear'; path = '/__xs/custom_metrics_clear'; prod = 'custom metrics clear api only available in xsdbg' },
		@{ name = 'reload_clear'; path = '/__xs/reload_clear'; prod = 'config reload clear api only available in xsdbg' },
		@{ name = 'reload_reset'; path = '/__xs/reload_reset'; prod = 'config reload reset api only available in xsdbg' },
		@{ name = 'check_config_clear'; path = '/__xs/check_config_clear'; prod = 'check config clear api only available in xsdbg' }
	)
	$arrDebugDisabledJson = @(
		@{ name = 'dashboard_json'; path = '/__xs/dashboard_json'; prod = 'dashboard json api only available in xsdbg' },
		@{ name = 'http_metrics_json'; path = '/__xs/http_metrics_json'; prod = 'http metrics json api only available in xsdbg' },
		@{ name = 'ws_metrics_json'; path = '/__xs/ws_metrics_json'; prod = 'ws metrics json api only available in xsdbg' },
		@{ name = 'xtp_metrics_json'; path = '/__xs/xtp_metrics_json'; prod = 'xtp metrics json api only available in xsdbg' },
		@{ name = 'udp_metrics_json'; path = '/__xs/udp_metrics_json'; prod = 'udp metrics json api only available in xsdbg' },
		@{ name = 'custom_metrics_json'; path = '/__xs/custom_metrics_json'; prod = 'custom metrics json api only available in xsdbg' }
	)
	$arrDebugReadOnlyText = @(
		@{ name = 'dashboard'; path = '/__xs/dashboard'; allow = 'GET, HEAD'; type = 'text/plain' },
		@{ name = 'http_metrics'; path = '/__xs/http_metrics'; allow = 'GET, HEAD'; type = 'text/plain' },
		@{ name = 'ws_metrics'; path = '/__xs/ws_metrics'; allow = 'GET, HEAD'; type = 'text/plain' },
		@{ name = 'xtp_metrics'; path = '/__xs/xtp_metrics'; allow = 'GET, HEAD'; type = 'text/plain' },
		@{ name = 'udp_metrics'; path = '/__xs/udp_metrics'; allow = 'GET, HEAD'; type = 'text/plain' },
		@{ name = 'custom_metrics'; path = '/__xs/custom_metrics'; allow = 'GET, HEAD'; type = 'text/plain' }
	)
	$arrDebugReadOnlyJson = @(
		@{ name = 'dashboard_json'; path = '/__xs/dashboard_json'; allow = 'GET, HEAD'; type = 'application/json' },
		@{ name = 'http_metrics_json'; path = '/__xs/http_metrics_json'; allow = 'GET, HEAD'; type = 'application/json' },
		@{ name = 'ws_metrics_json'; path = '/__xs/ws_metrics_json'; allow = 'GET, HEAD'; type = 'application/json' },
		@{ name = 'xtp_metrics_json'; path = '/__xs/xtp_metrics_json'; allow = 'GET, HEAD'; type = 'application/json' },
		@{ name = 'udp_metrics_json'; path = '/__xs/udp_metrics_json'; allow = 'GET, HEAD'; type = 'application/json' },
		@{ name = 'custom_metrics_json'; path = '/__xs/custom_metrics_json'; allow = 'GET, HEAD'; type = 'application/json' }
	)
	$arrDebugGetOnly = @(
		@{ name = 'http_metrics_clear'; path = '/__xs/http_metrics_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'ws_metrics_clear'; path = '/__xs/ws_metrics_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'xtp_metrics_clear'; path = '/__xs/xtp_metrics_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'udp_metrics_clear'; path = '/__xs/udp_metrics_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'custom_metrics_clear'; path = '/__xs/custom_metrics_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'reload_clear'; path = '/__xs/reload_clear'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'reload_reset'; path = '/__xs/reload_reset'; allow = 'GET'; type = 'text/plain' },
		@{ name = 'check_config_clear'; path = '/__xs/check_config_clear'; allow = 'GET'; type = 'text/plain' }
	)
	$i;
	$tblMetric;
	$tblAPI;

	if ( -not (Test-Path $sExePath) ) {
		$arrResults.Add("FAIL $sExeName : executable not found")
		return @{
			code = 1
			lines = $arrResults
		}
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList $sConfig -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden

	try {
		if ( -not (procWaitReady) ) {
			$arrResults.Add("FAIL $sExeName : server not ready within ${iWaitMS}ms")
			return @{
				code = 1
				lines = $arrResults
			}
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/status_json"
		procCheck $arrResults "$sExeName status_json" $objResp.status 200 $objResp.body '"manage_api":true'
		procCheckBodyContains $arrResults "$sExeName status_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "Content-Type") 'application/json'
		procCheckBodyContains $arrResults "$sExeName status_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName status_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName status_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName status_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/status"
		procCheck $arrResults "$sExeName status" $objResp.status 200 $objResp.body 'manage_api=true'
		procCheckBodyContains $arrResults "$sExeName status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName status cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName status frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName status referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName status nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/health_json"
		procCheck $arrResults "$sExeName health_json" $objResp.status 200 $objResp.body '"ok":true'
		procCheckBodyContains $arrResults "$sExeName health_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "Content-Type") 'application/json'
		procCheckBodyContains $arrResults "$sExeName health_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName health_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName health_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName health_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/health"
		procCheck $arrResults "$sExeName health" $objResp.status 200 $objResp.body 'ok=true'
		procCheckBodyContains $arrResults "$sExeName health type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName health cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName health frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName health referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName health nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_status_json"
		procCheck $arrResults "$sExeName reload_status_json" $objResp.status 200 $objResp.body '"busy":false'
		procCheckBodyContains $arrResults "$sExeName reload_status_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Content-Type") 'application/json'
		procCheckBodyContains $arrResults "$sExeName reload_status_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName reload_status_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName reload_status_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName reload_status_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_status"
		procCheck $arrResults "$sExeName reload_status" $objResp.status 200 $objResp.body 'busy=false'
		procCheckBodyContains $arrResults "$sExeName reload_status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName reload_status cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName reload_status frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName reload_status referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName reload_status nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload"
		procCheck $arrResults "$sExeName reload" $objResp.status 200 $objResp.body 'result=true'
		procCheckBodyContains $arrResults "$sExeName reload type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName reload cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName reload frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName reload referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName reload nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "X-Content-Type-Options") 'nosniff'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_idle_after_text : timeout")
		}

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload"
		procCheck $arrResults "$sExeName post_reload" $objResp.status 200 $objResp.body 'result=true'
		procCheckBodyContains $arrResults "$sExeName post_reload type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName post_reload cache" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName post_reload frame" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName post_reload referrer" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName post_reload nosniff" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "X-Content-Type-Options") 'nosniff'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_idle_after_text_post : timeout")
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_config"
		procCheck $arrResults "$sExeName reload_config" $objResp.status 200 $objResp.body 'result=true'
		procCheckBodyContains $arrResults "$sExeName reload_config body" $objResp.body 'config reload queued'
		procCheckBodyContains $arrResults "$sExeName reload_config type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName reload_config cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName reload_config frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName reload_config referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName reload_config nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "X-Content-Type-Options") 'nosniff'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_idle_after_text : timeout")
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_text_post_idle_wait : timeout")
		} else {
			$iReloadConfigTextTry = 0
			$bReloadConfigTextOK = $false
			while ( $iReloadConfigTextTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_config"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName post_reload_config" $objResp.status 200 $objResp.body 'result=true'
					procCheckBodyContains $arrResults "$sExeName post_reload_config body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName post_reload_config type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "Content-Type") 'text/plain'
					procCheckBodyContains $arrResults "$sExeName post_reload_config cache" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "Cache-Control") 'no-store'
					procCheckBodyContains $arrResults "$sExeName post_reload_config frame" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "X-Frame-Options") 'DENY'
					procCheckBodyContains $arrResults "$sExeName post_reload_config referrer" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "Referrer-Policy") 'no-referrer'
					procCheckBodyContains $arrResults "$sExeName post_reload_config nosniff" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "X-Content-Type-Options") 'nosniff'
					$bReloadConfigTextOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName post_reload_config" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTextTry++
			}

			if ( -not $bReloadConfigTextOK -and $iReloadConfigTextTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName post_reload_config : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_idle_after_text_post : timeout")
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config_json"
		procCheck $arrResults "$sExeName check_config_json" $objResp.status 200 $objResp.body '"check_total_count":'
		procCheckBodyContains $arrResults "$sExeName check_config_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "Content-Type") 'application/json'
		procCheckBodyContains $arrResults "$sExeName check_config_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName check_config_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName check_config_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName check_config_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config"
		procCheck $arrResults "$sExeName check_config" $objResp.status 200 $objResp.body 'check_total_count='
		procCheckBodyContains $arrResults "$sExeName check_config type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "Content-Type") 'text/plain'
		procCheckBodyContains $arrResults "$sExeName check_config cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "Cache-Control") 'no-store'
		procCheckBodyContains $arrResults "$sExeName check_config frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "X-Frame-Options") 'DENY'
		procCheckBodyContains $arrResults "$sExeName check_config referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "Referrer-Policy") 'no-referrer'
		procCheckBodyContains $arrResults "$sExeName check_config nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "X-Content-Type-Options") 'nosniff'

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/status_json"
		procCheck $arrResults "$sExeName post_status_json" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_status_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/status_json") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_status_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName post_status_json" "POST" "http://127.0.0.1:$iPort/__xs/status_json"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/status"
		procCheck $arrResults "$sExeName post_status" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_status allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/status") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_status type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName post_status" "POST" "http://127.0.0.1:$iPort/__xs/status"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/health_json"
		procCheck $arrResults "$sExeName post_health_json" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_health_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/health_json") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_health_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/health_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName post_health_json" "POST" "http://127.0.0.1:$iPort/__xs/health_json"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/health"
		procCheck $arrResults "$sExeName post_health" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_health allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/health") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_health type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/health" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName post_health" "POST" "http://127.0.0.1:$iPort/__xs/health"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_status_json"
		procCheck $arrResults "$sExeName post_reload_status_json" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_reload_status_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_status_json") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_reload_status_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName post_reload_status_json" "POST" "http://127.0.0.1:$iPort/__xs/reload_status_json"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_status"
		procCheck $arrResults "$sExeName post_reload_status" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_reload_status allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_status") 'GET, HEAD'
		procCheckBodyContains $arrResults "$sExeName post_reload_status type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName post_reload_status" "POST" "http://127.0.0.1:$iPort/__xs/reload_status"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/check_config_json"
		procCheck $arrResults "$sExeName post_check_config_json" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_check_config_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config_json") 'GET'
		procCheckBodyContains $arrResults "$sExeName post_check_config_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName post_check_config_json" "POST" "http://127.0.0.1:$iPort/__xs/check_config_json"

		$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/check_config"
		procCheck $arrResults "$sExeName post_check_config" $objResp.status 405 $objResp.body
		procCheckBodyContains $arrResults "$sExeName post_check_config allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config") 'GET'
		procCheckBodyContains $arrResults "$sExeName post_check_config type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName post_check_config" "POST" "http://127.0.0.1:$iPort/__xs/check_config"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/status_json"
		procCheck $arrResults "$sExeName head_status_json" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_status_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_status_json" "HEAD" "http://127.0.0.1:$iPort/__xs/status_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/status"
		procCheck $arrResults "$sExeName head_status" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_status type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_status" "HEAD" "http://127.0.0.1:$iPort/__xs/status"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/health_json"
		procCheck $arrResults "$sExeName head_health_json" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_health_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/health_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_health_json" "HEAD" "http://127.0.0.1:$iPort/__xs/health_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/health"
		procCheck $arrResults "$sExeName head_health" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_health type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/health" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_health" "HEAD" "http://127.0.0.1:$iPort/__xs/health"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status_json"
		procCheck $arrResults "$sExeName head_reload_status_json" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_reload_status_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload_status_json" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status"
		procCheck $arrResults "$sExeName head_reload_status" $iStatus 200 ""
		procCheckBodyContains $arrResults "$sExeName head_reload_status type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload_status" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_status"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config"
		procCheck $arrResults "$sExeName head_check_config" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_check_config allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config") 'GET'
		procCheckBodyContains $arrResults "$sExeName head_check_config type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_check_config" "HEAD" "http://127.0.0.1:$iPort/__xs/check_config"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_json"
		procCheck $arrResults "$sExeName head_check_config_json" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_check_config_json allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_json") 'GET'
		procCheckBodyContains $arrResults "$sExeName head_check_config_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_check_config_json" "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_json"
		procCheck $arrResults "$sExeName head_reload_json" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_reload_json allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_json") 'GET, POST'
		procCheckBodyContains $arrResults "$sExeName head_reload_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload_json" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload"
		procCheck $arrResults "$sExeName head_reload" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_reload allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload") 'GET, POST'
		procCheckBodyContains $arrResults "$sExeName head_reload type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload" "HEAD" "http://127.0.0.1:$iPort/__xs/reload"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config_json"
		procCheck $arrResults "$sExeName head_reload_config_json" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_reload_config_json allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config_json") 'GET, POST'
		procCheckBodyContains $arrResults "$sExeName head_reload_config_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload_config_json" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config_json"

		$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config"
		procCheck $arrResults "$sExeName head_reload_config" $iStatus 405 ""
		procCheckBodyContains $arrResults "$sExeName head_reload_config allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config") 'GET, POST'
		procCheckBodyContains $arrResults "$sExeName head_reload_config type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName head_reload_config" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_config"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/unknown"
		procCheck $arrResults "$sExeName unknown_manage" $objResp.status 404 $objResp.body
	procCheckBodyContains $arrResults "$sExeName unknown_manage type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/unknown" "Content-Type") 'text/plain'
	procCheckSecurityHeaders $arrResults "$sExeName unknown_manage" "GET" "http://127.0.0.1:$iPort/__xs/unknown"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/unknown_json"
		procCheck $arrResults "$sExeName unknown_manage_json" $objResp.status 404 $objResp.body
	procCheckBodyContains $arrResults "$sExeName unknown_manage_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/unknown_json" "Content-Type") 'application/json'
	procCheckSecurityHeaders $arrResults "$sExeName unknown_manage_json" "GET" "http://127.0.0.1:$iPort/__xs/unknown_json"

		procCheckDisabledEndpoint $arrResults $sExeName "manage_root" "GET" "/__xs" 404 'text/plain' 'manage api not found'
		procCheckDisabledEndpoint $arrResults $sExeName "head_manage_root" "HEAD" "/__xs" 404 'text/plain' $null
		procCheckDisabledEndpoint $arrResults $sExeName "post_manage_root" "POST" "/__xs" 404 'text/plain' 'manage api not found'
		procCheckDisabledEndpoint $arrResults $sExeName "head_unknown_manage" "HEAD" "/__xs/unknown" 404 'text/plain' $null
		procCheckDisabledEndpoint $arrResults $sExeName "post_unknown_manage" "POST" "/__xs/unknown" 404 'text/plain' 'manage api not found'
		procCheckDisabledEndpoint $arrResults $sExeName "head_unknown_manage_json" "HEAD" "/__xs/unknown_json" 404 'application/json' $null
		procCheckDisabledEndpoint $arrResults $sExeName "post_unknown_manage_json" "POST" "/__xs/unknown_json" 404 'application/json' 'manage api not found'

		$objResp = procFetch "http://127.0.0.1:$iPort/"
		procCheck $arrResults "$sExeName root" $objResp.status 200 $objResp.body

		$objResp = procFetch "http://127.0.0.1:$iPort/json"
		procCheck $arrResults "$sExeName json_route" $objResp.status 200 $objResp.body '"path":"/json"'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_idle_wait : timeout")
		} else {
			$iReloadTry = 0
			$bReloadOK = $false
			while ( $iReloadTry -lt 10 ) {
				$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName reload_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName reload_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "Content-Type") 'application/json'
					procCheckBodyContains $arrResults "$sExeName reload_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "Cache-Control") 'no-store'
					procCheckBodyContains $arrResults "$sExeName reload_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "X-Frame-Options") 'DENY'
					procCheckBodyContains $arrResults "$sExeName reload_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "Referrer-Policy") 'no-referrer'
					procCheckBodyContains $arrResults "$sExeName reload_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "X-Content-Type-Options") 'nosniff'
					$bReloadOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName reload_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadTry++
			}

			if ( -not $bReloadOK -and $iReloadTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName reload_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_post_idle_wait : timeout")
		} else {
			$iReloadTry = 0
			$bReloadOK = $false
			while ( $iReloadTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName post_reload_json" $objResp.status 200 $objResp.body '"result":true'
					$bReloadOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName post_reload_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadTry++
			}

			if ( -not $bReloadOK -and $iReloadTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName post_reload_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_idle_after : timeout")
		}

		$objResp = procWaitBodyContains "http://127.0.0.1:$iPort/__xs/reload_status_json" '"busy":false'
		if ( $null -eq $objResp ) {
			$arrResults.Add("FAIL $sExeName reload_status_after : missing text '""busy"":false'")
		} else {
			procCheck $arrResults "$sExeName reload_status_after" $objResp.status 200 $objResp.body '"busy":false'
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/json"
		procCheck $arrResults "$sExeName json_route_after_reload" $objResp.status 200 $objResp.body '"path":"/json"'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_idle_wait : timeout")
		} else {
			$iReloadConfigTry = 0
			$bReloadConfigOK = $false
			while ( $iReloadConfigTry -lt 10 ) {
				$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_config_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName reload_config_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName reload_config_json body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName reload_config_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Content-Type") 'application/json'
					procCheckBodyContains $arrResults "$sExeName reload_config_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Cache-Control") 'no-store'
					procCheckBodyContains $arrResults "$sExeName reload_config_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "X-Frame-Options") 'DENY'
					procCheckBodyContains $arrResults "$sExeName reload_config_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Referrer-Policy") 'no-referrer'
					procCheckBodyContains $arrResults "$sExeName reload_config_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "X-Content-Type-Options") 'nosniff'
					$bReloadConfigOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName reload_config_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTry++
			}

			if ( -not $bReloadConfigOK -and $iReloadConfigTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName reload_config_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_post_idle_wait : timeout")
		} else {
			$iReloadConfigTry = 0
			$bReloadConfigOK = $false
			while ( $iReloadConfigTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_config_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName post_reload_config_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName post_reload_config_json body" $objResp.body 'config reload queued'
					$bReloadConfigOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName post_reload_config_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTry++
			}

			if ( -not $bReloadConfigOK -and $iReloadConfigTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName post_reload_config_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName reload_config_idle_after : timeout")
		}

		$objResp = procWaitBodyContains "http://127.0.0.1:$iPort/__xs/reload_status_json" '"busy":false'
		if ( $null -eq $objResp ) {
			$arrResults.Add("FAIL $sExeName reload_status_after_config : missing text '""busy"":false'")
		} else {
			procCheck $arrResults "$sExeName reload_status_after_config" $objResp.status 200 $objResp.body '"busy":false'
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/json"
		procCheck $arrResults "$sExeName json_route_after_config_reload" $objResp.status 200 $objResp.body '"path":"/json"'

		if ( $bDebug ) {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName dashboard" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName dashboard body" $objResp.body 'http_req_count='
			procCheckBodyContains $arrResults "$sExeName dashboard type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName dashboard" "GET" "http://127.0.0.1:$iPort/__xs/dashboard"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName head_dashboard" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName head_dashboard type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName head_dashboard" "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName dashboard_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName dashboard_json body" $objResp.body '"status"'
			procCheckBodyContains $arrResults "$sExeName dashboard_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName dashboard_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName dashboard_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName dashboard_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName dashboard_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "X-Content-Type-Options") 'nosniff'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName head_dashboard_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName head_dashboard_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_dashboard_json" "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json"

			procCheckDisabledEndpoint $arrResults $sExeName "bus_root" "GET" "/__xs/bus" 404 'application/json' 'manage api not found'
			procCheckDisabledEndpoint $arrResults $sExeName "head_bus_root" "HEAD" "/__xs/bus" 404 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "post_bus_root" "POST" "/__xs/bus" 404 'application/json' 'manage api not found'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName bus_status" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_status body" $objResp.body '"data_count"'
			procCheckBodyContains $arrResults "$sExeName bus_status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName bus_status cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName bus_status frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName bus_status referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName bus_status nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "X-Content-Type-Options") 'nosniff'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName head_bus_status" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_status allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_status type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_status" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName post_bus_status" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_status allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_status type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_status" "POST" "http://127.0.0.1:$iPort/__xs/bus/status"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName bus_namespaces" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_namespaces body" $objResp.body '"items"'
			procCheckBodyContains $arrResults "$sExeName bus_namespaces type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName bus_namespaces" "GET" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName head_bus_namespaces" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_namespaces allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_namespaces type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_namespaces" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName post_bus_namespaces" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_namespaces allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_namespaces type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_namespaces" "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName bus_registry" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_registry body" $objResp.body '"items"'
			procCheckBodyContains $arrResults "$sExeName bus_registry type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName bus_registry" "GET" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName head_bus_registry" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_registry allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_registry type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_registry" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName post_bus_registry" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_registry allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_registry type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_registry" "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName bus_limits" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_limits body" $objResp.body '"data_limit"'
			procCheckBodyContains $arrResults "$sExeName bus_limits type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName bus_limits" "GET" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName head_bus_limits" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_limits allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_limits type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_limits" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName post_bus_limits" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_limits allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_limits type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_limits" "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName bus_send" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_send body" $objResp.body '"result":true'
			procCheckBodyContains $arrResults "$sExeName bus_send type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName bus_send" "GET" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName head_bus_send" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_send allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_send type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_send" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName post_bus_send" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_send allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_send type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_send" "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName bus_reset" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_reset body" $objResp.body '"sweep_count"'
			procCheckBodyContains $arrResults "$sExeName bus_reset type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName bus_reset" "GET" "http://127.0.0.1:$iPort/__xs/bus/reset"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName head_bus_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_reset allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_bus_reset type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_reset" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName post_bus_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_reset allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName post_bus_reset type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_reset" "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"

			for ( $i = 0; $i -lt $arrMetricsClearText.Length; $i++ ) {
				$tblMetric = $arrMetricsClearText[$i]
				$objResp = procFetch ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName $($tblMetric.name)" $objResp.status 200 $objResp.body
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) body" $objResp.body $tblMetric.token
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) type" (procFetchHeaderMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName $($tblMetric.name)" "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path)
			}

			for ( $i = 0; $i -lt $arrMetricsText.Length; $i++ ) {
				$tblMetric = $arrMetricsText[$i]
				$objResp = procFetch ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName $($tblMetric.name)" $objResp.status 200 $objResp.body
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) body" $objResp.body $tblMetric.token
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) type" (procFetchHeaderMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName $($tblMetric.name)" "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path)

				$iStatus = procFetchStatusMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName head_$($tblMetric.name)" $iStatus 200 ""
				procCheckBodyContains $arrResults "$sExeName head_$($tblMetric.name) type" (procFetchHeaderMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName head_$($tblMetric.name)" "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)
			}

			for ( $i = 0; $i -lt $arrMetricsJson.Length; $i++ ) {
				$tblMetric = $arrMetricsJson[$i]
				$objResp = procFetch ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName $($tblMetric.name)" $objResp.status 200 $objResp.body
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) body" $objResp.body $tblMetric.token
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) type" (procFetchHeaderMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'application/json'
				procCheckSecurityHeaders $arrResults "$sExeName $($tblMetric.name)" "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path)

				$iStatus = procFetchStatusMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName head_$($tblMetric.name)" $iStatus 200 ""
				procCheckBodyContains $arrResults "$sExeName head_$($tblMetric.name) type" (procFetchHeaderMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'application/json'
				procCheckSecurityHeaders $arrResults "$sExeName head_$($tblMetric.name)" "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)
			}

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName reload_clear" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName reload_clear body" $objResp.body 'reload_total_count='
			procCheckBodyContains $arrResults "$sExeName reload_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName reload_clear" "GET" "http://127.0.0.1:$iPort/__xs/reload_clear"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName reload_reset" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName reload_reset body" $objResp.body 'reload_total_count='
			procCheckBodyContains $arrResults "$sExeName reload_reset type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName reload_reset" "GET" "http://127.0.0.1:$iPort/__xs/reload_reset"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName check_config_clear" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName check_config_clear body" $objResp.body 'check_total_count='
			procCheckBodyContains $arrResults "$sExeName check_config_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName check_config_clear" "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName head_reload_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_reload_clear allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_reload_clear type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName head_reload_clear" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName head_reload_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_reload_reset allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_reload_reset type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName head_reload_reset" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName head_check_config_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName head_check_config_clear allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName head_check_config_clear type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName head_check_config_clear" "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear"

			foreach ( $tblAPI in $arrDebugReadOnlyText ) {
				procCheckMethodReject $arrResults $sExeName ("post_" + $tblAPI.name) "POST" $tblAPI.path 405 $tblAPI.allow $tblAPI.type
			}
			foreach ( $tblAPI in $arrDebugReadOnlyJson ) {
				procCheckMethodReject $arrResults $sExeName ("post_" + $tblAPI.name) "POST" $tblAPI.path 405 $tblAPI.allow $tblAPI.type
			}
			foreach ( $tblAPI in $arrDebugGetOnly ) {
				procCheckMethodReject $arrResults $sExeName ("post_" + $tblAPI.name) "POST" $tblAPI.path 405 $tblAPI.allow $tblAPI.type
			}
		} else {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName dashboard" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName dashboard body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard") 'dashboard api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName dashboard type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName dashboard cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName dashboard frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName dashboard referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName dashboard nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName dashboard_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName dashboard_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json") 'dashboard json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName dashboard_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName dashboard_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName dashboard_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName dashboard_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName dashboard_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "X-Content-Type-Options") 'nosniff'

			procCheckDisabledEndpoint $arrResults $sExeName "bus_root" "GET" "/__xs/bus" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "head_bus_root_disabled" "HEAD" "/__xs/bus" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "post_bus_root_disabled" "POST" "/__xs/bus" 403 'application/json' 'bus api not included in production xs'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName bus_status" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_status body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName bus_status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName bus_status cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName bus_status frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName bus_status referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName bus_status nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "X-Content-Type-Options") 'nosniff'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName head_bus_status_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_status_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_status_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName post_bus_status_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_status_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_status_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_status_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/status"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName bus_namespaces" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_namespaces body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'bus api not included in production xs'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName head_bus_namespaces_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_namespaces_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_namespaces_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName post_bus_namespaces_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_namespaces_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_namespaces_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_namespaces_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName bus_registry" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_registry body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/registry") 'bus api not included in production xs'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName head_bus_registry_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_registry_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_registry_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName post_bus_registry_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_registry_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_registry_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_registry_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName bus_limits" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_limits body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/limits") 'bus api not included in production xs'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName head_bus_limits_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_limits_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_limits_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName post_bus_limits_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_limits_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_limits_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_limits_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName bus_send" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_send body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'bus api not included in production xs'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName head_bus_send_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_send_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_send_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName post_bus_send_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_send_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_send_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_send_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName bus_reset" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName bus_reset body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/reset") 'bus api not included in production xs'

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName head_bus_reset_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName head_bus_reset_disabled type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName head_bus_reset_disabled" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName post_bus_reset_disabled" $iStatus 403 ""
			procCheckBodyContains $arrResults "$sExeName post_bus_reset_disabled body" (procFetchBodyMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset") 'bus api not included in production xs'
			procCheckBodyContains $arrResults "$sExeName post_bus_reset_disabled type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName post_bus_reset_disabled" "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"

			for ( $i = 0; $i -lt $arrMetricsClear.Length; $i++ ) {
				$tblMetric = $arrMetricsClear[$i]
				$objResp = procFetch ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName $($tblMetric.name)" $objResp.status 403 $objResp.body
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) body" (procFetchBodyMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path)) $tblMetric.prod
				procCheckBodyContains $arrResults "$sExeName $($tblMetric.name) type" (procFetchHeaderMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
			}

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/http_metrics"
			procCheck $arrResults "$sExeName http_metrics" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName http_metrics body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics") 'http metrics api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName http_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName http_metrics cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName http_metrics frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName http_metrics referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName http_metrics nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/http_metrics_json"
			procCheck $arrResults "$sExeName http_metrics_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName http_metrics_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json") 'http metrics json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName http_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName http_metrics_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName http_metrics_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName http_metrics_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName http_metrics_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/ws_metrics"
			procCheck $arrResults "$sExeName ws_metrics" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName ws_metrics body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics") 'ws metrics api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName ws_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName ws_metrics cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName ws_metrics frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName ws_metrics referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName ws_metrics nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/ws_metrics_json"
			procCheck $arrResults "$sExeName ws_metrics_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json") 'ws metrics json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName ws_metrics_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/xtp_metrics"
			procCheck $arrResults "$sExeName xtp_metrics" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName xtp_metrics body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics") 'xtp metrics api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"
			procCheck $arrResults "$sExeName xtp_metrics_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json") 'xtp metrics json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName xtp_metrics_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/udp_metrics"
			procCheck $arrResults "$sExeName udp_metrics" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName udp_metrics body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics") 'udp metrics api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName udp_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName udp_metrics cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName udp_metrics frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName udp_metrics referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName udp_metrics nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/udp_metrics_json"
			procCheck $arrResults "$sExeName udp_metrics_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json") 'udp metrics json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName udp_metrics_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/custom_metrics"
			procCheck $arrResults "$sExeName custom_metrics" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName custom_metrics body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics") 'custom metrics api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName custom_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName custom_metrics cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName custom_metrics frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName custom_metrics referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName custom_metrics nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/custom_metrics_json"
			procCheck $arrResults "$sExeName custom_metrics_json" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json") 'custom metrics json api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Content-Type") 'application/json'
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName custom_metrics_json nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName reload_clear" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName reload_clear body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear") 'config reload clear api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName reload_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName reload_clear cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName reload_clear frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName reload_clear referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName reload_clear nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName reload_reset" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName reload_reset body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset") 'config reload reset api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName reload_reset type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName reload_reset cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName reload_reset frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName reload_reset referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName reload_reset nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "X-Content-Type-Options") 'nosniff'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName check_config_clear" $objResp.status 403 $objResp.body
			procCheckBodyContains $arrResults "$sExeName check_config_clear body" (procFetchBodyMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear") 'check config clear api only available in xsdbg'
			procCheckBodyContains $arrResults "$sExeName check_config_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckBodyContains $arrResults "$sExeName check_config_clear cache" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Cache-Control") 'no-store'
			procCheckBodyContains $arrResults "$sExeName check_config_clear frame" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "X-Frame-Options") 'DENY'
			procCheckBodyContains $arrResults "$sExeName check_config_clear referrer" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Referrer-Policy") 'no-referrer'
			procCheckBodyContains $arrResults "$sExeName check_config_clear nosniff" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "X-Content-Type-Options") 'nosniff'

			foreach ( $tblAPI in $arrDebugDisabledText ) {
				procCheckDisabledEndpoint $arrResults $sExeName ("head_" + $tblAPI.name + "_disabled") "HEAD" $tblAPI.path 403 'text/plain' $null
				procCheckDisabledEndpoint $arrResults $sExeName ("post_" + $tblAPI.name + "_disabled") "POST" $tblAPI.path 403 'text/plain' $tblAPI.prod
			}
			foreach ( $tblAPI in $arrDebugDisabledJson ) {
				procCheckDisabledEndpoint $arrResults $sExeName ("head_" + $tblAPI.name + "_disabled") "HEAD" $tblAPI.path 403 'application/json' $null
				procCheckDisabledEndpoint $arrResults $sExeName ("post_" + $tblAPI.name + "_disabled") "POST" $tblAPI.path 403 'application/json' $tblAPI.prod
			}
		}

		$iCode = 0
		foreach ( $sLine in $arrResults ) {
			if ( $sLine.StartsWith("FAIL ") ) {
				$iCode = 1
				break
			}
		}

		return @{
			code = $iCode
			lines = $arrResults
		}
	} finally {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		Start-Sleep -Milliseconds 500
	}
}






function procRunRepoRootCase([string]$sExeName, [bool]$bDebug)
{
	$sExePath = Join-Path $sReleaseDir $sExeName
	$arrResults = New-Object 'System.Collections.Generic.List[string]'

	if ( -not (Test-Path $sExePath) ) {
		$arrResults.Add("FAIL $sExeName repo_root : executable not found")
		return @{ code = 1; lines = $arrResults }
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList $sConfig -WorkingDirectory $sRepoRoot -PassThru -WindowStyle Hidden

	try {
		if ( -not (procWaitReady) ) {
			$arrResults.Add("FAIL $sExeName repo_root_ready : server not ready within ${iWaitMS}ms")
			return @{ code = 1; lines = $arrResults }
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/status_json"
		procCheck $arrResults "$sExeName repo_root status_json" $objResp.status 200 $objResp.body '"manage_api":true'
		procCheckBodyContains $arrResults "$sExeName repo_root status_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root status_json" "GET" "http://127.0.0.1:$iPort/__xs/status_json"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/status"
		procCheck $arrResults "$sExeName repo_root status" $objResp.status 200 $objResp.body 'manage_api=true'
		procCheckBodyContains $arrResults "$sExeName repo_root status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root status" "GET" "http://127.0.0.1:$iPort/__xs/status"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/health_json"
		procCheck $arrResults "$sExeName repo_root health_json" $objResp.status 200 $objResp.body '"ok":true'
		procCheckBodyContains $arrResults "$sExeName repo_root health_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root health_json" "GET" "http://127.0.0.1:$iPort/__xs/health_json"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/health"
		procCheck $arrResults "$sExeName repo_root health" $objResp.status 200 $objResp.body 'ok=true'
		procCheckBodyContains $arrResults "$sExeName repo_root health type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/health" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root health" "GET" "http://127.0.0.1:$iPort/__xs/health"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_status_json"
		procCheck $arrResults "$sExeName repo_root reload_status_json" $objResp.status 200 $objResp.body '"busy":false'
		procCheckBodyContains $arrResults "$sExeName repo_root reload_status_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_status_json" "GET" "http://127.0.0.1:$iPort/__xs/reload_status_json"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_status"
		procCheck $arrResults "$sExeName repo_root reload_status" $objResp.status 200 $objResp.body 'busy=false'
		procCheckBodyContains $arrResults "$sExeName repo_root reload_status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_status" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_status" "GET" "http://127.0.0.1:$iPort/__xs/reload_status"

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_idle_wait : timeout")
		} else {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload"
			procCheck $arrResults "$sExeName repo_root reload" $objResp.status 200 $objResp.body 'result=true'
			procCheckBodyContains $arrResults "$sExeName repo_root reload type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root reload" "GET" "http://127.0.0.1:$iPort/__xs/reload"
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_idle_after : timeout")
		} else {
			$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload"
			procCheck $arrResults "$sExeName repo_root post_reload" $objResp.status 200 $objResp.body 'result=true'
			procCheckBodyContains $arrResults "$sExeName repo_root post_reload type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload" "POST" "http://127.0.0.1:$iPort/__xs/reload"
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_post_idle_wait : timeout")
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_json_idle_wait : timeout")
		} else {
			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_json"
			procCheck $arrResults "$sExeName repo_root reload_json" $objResp.status 200 $objResp.body '"result":true'
			procCheckBodyContains $arrResults "$sExeName repo_root reload_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_json" "GET" "http://127.0.0.1:$iPort/__xs/reload_json"
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_json_idle_after : timeout")
		} else {
			$iReloadTry = 0
			$bReloadOK = $false
			while ( $iReloadTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName repo_root post_reload_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_json" "Content-Type") 'application/json'
					procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload_json" "POST" "http://127.0.0.1:$iPort/__xs/reload_json"
					$bReloadOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadTry++
			}

			if ( -not $bReloadOK -and $iReloadTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName repo_root post_reload_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_json_post_idle_wait : timeout")
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config_json"
		procCheck $arrResults "$sExeName repo_root check_config_json" $objResp.status 200 $objResp.body '"check_total_count":'
		procCheckBodyContains $arrResults "$sExeName repo_root check_config_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_json" "Content-Type") 'application/json'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root check_config_json" "GET" "http://127.0.0.1:$iPort/__xs/check_config_json"

		$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config"
		procCheck $arrResults "$sExeName repo_root check_config" $objResp.status 200 $objResp.body 'check_total_count='
		procCheckBodyContains $arrResults "$sExeName repo_root check_config type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config" "Content-Type") 'text/plain'
		procCheckSecurityHeaders $arrResults "$sExeName repo_root check_config" "GET" "http://127.0.0.1:$iPort/__xs/check_config"

		$arrRepoRootHeadOK = @(
			@{ name = 'head_status_json'; path = '/__xs/status_json'; type = 'application/json' },
			@{ name = 'head_status'; path = '/__xs/status'; type = 'text/plain' },
			@{ name = 'head_health_json'; path = '/__xs/health_json'; type = 'application/json' },
			@{ name = 'head_health'; path = '/__xs/health'; type = 'text/plain' },
			@{ name = 'head_reload_status_json'; path = '/__xs/reload_status_json'; type = 'application/json' },
			@{ name = 'head_reload_status'; path = '/__xs/reload_status'; type = 'text/plain' }
		)
		$arrRepoRootMethodReject = @(
			@{ name = 'post_status_json'; method = 'POST'; path = '/__xs/status_json'; status = 405; allow = 'GET, HEAD'; type = 'application/json' },
			@{ name = 'post_status'; method = 'POST'; path = '/__xs/status'; status = 405; allow = 'GET, HEAD'; type = 'text/plain' },
			@{ name = 'post_health_json'; method = 'POST'; path = '/__xs/health_json'; status = 405; allow = 'GET, HEAD'; type = 'application/json' },
			@{ name = 'post_health'; method = 'POST'; path = '/__xs/health'; status = 405; allow = 'GET, HEAD'; type = 'text/plain' },
			@{ name = 'post_reload_status_json'; method = 'POST'; path = '/__xs/reload_status_json'; status = 405; allow = 'GET, HEAD'; type = 'application/json' },
			@{ name = 'post_reload_status'; method = 'POST'; path = '/__xs/reload_status'; status = 405; allow = 'GET, HEAD'; type = 'text/plain' },
			@{ name = 'post_check_config_json'; method = 'POST'; path = '/__xs/check_config_json'; status = 405; allow = 'GET'; type = 'application/json' },
			@{ name = 'post_check_config'; method = 'POST'; path = '/__xs/check_config'; status = 405; allow = 'GET'; type = 'text/plain' },
			@{ name = 'head_check_config_json'; method = 'HEAD'; path = '/__xs/check_config_json'; status = 405; allow = 'GET'; type = 'application/json' },
			@{ name = 'head_check_config'; method = 'HEAD'; path = '/__xs/check_config'; status = 405; allow = 'GET'; type = 'text/plain' },
			@{ name = 'head_reload_json'; method = 'HEAD'; path = '/__xs/reload_json'; status = 405; allow = 'GET, POST'; type = 'application/json' },
			@{ name = 'head_reload'; method = 'HEAD'; path = '/__xs/reload'; status = 405; allow = 'GET, POST'; type = 'text/plain' },
			@{ name = 'head_reload_config_json'; method = 'HEAD'; path = '/__xs/reload_config_json'; status = 405; allow = 'GET, POST'; type = 'application/json' },
			@{ name = 'head_reload_config'; method = 'HEAD'; path = '/__xs/reload_config'; status = 405; allow = 'GET, POST'; type = 'text/plain' }
		)

		foreach ( $tblAPI in $arrRepoRootHeadOK ) {
			$iStatus = procFetchStatusMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblAPI.path)
			procCheck $arrResults "$sExeName repo_root $($tblAPI.name)" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root $($tblAPI.name) type" (procFetchHeaderMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblAPI.path) "Content-Type") $tblAPI.type
			procCheckSecurityHeaders $arrResults "$sExeName repo_root $($tblAPI.name)" "HEAD" ("http://127.0.0.1:$iPort" + $tblAPI.path)
		}

		foreach ( $tblAPI in $arrRepoRootMethodReject ) {
			procCheckMethodReject $arrResults $sExeName ("repo_root " + $tblAPI.name) $tblAPI.method $tblAPI.path $tblAPI.status $tblAPI.allow $tblAPI.type
		}

		procCheckDisabledEndpoint $arrResults $sExeName "repo_root manage_root" "GET" "/__xs" 404 'text/plain' 'manage api not found'
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_manage_root" "HEAD" "/__xs" 404 'text/plain' $null
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_manage_root" "POST" "/__xs" 404 'text/plain' 'manage api not found'

		procCheckDisabledEndpoint $arrResults $sExeName "repo_root unknown_manage" "GET" "/__xs/unknown" 404 'text/plain' $null
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_unknown_manage" "HEAD" "/__xs/unknown" 404 'text/plain' $null
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_unknown_manage" "POST" "/__xs/unknown" 404 'text/plain' 'manage api not found'

		procCheckDisabledEndpoint $arrResults $sExeName "repo_root unknown_manage_json" "GET" "/__xs/unknown_json" 404 'application/json' $null
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_unknown_manage_json" "HEAD" "/__xs/unknown_json" 404 'application/json' $null
		procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_unknown_manage_json" "POST" "/__xs/unknown_json" 404 'application/json' 'manage api not found'

		$objResp = procFetch "http://127.0.0.1:$iPort/json"
		procCheck $arrResults "$sExeName repo_root json_route" $objResp.status 200 $objResp.body '"path":"/json"'

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_config_idle_wait : timeout")
		} else {
			$iReloadConfigTextTry = 0
			$bReloadConfigTextOK = $false
			while ( $iReloadConfigTextTry -lt 10 ) {
				$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_config"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName repo_root reload_config" $objResp.status 200 $objResp.body 'result=true'
					procCheckBodyContains $arrResults "$sExeName repo_root reload_config body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName repo_root reload_config type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config" "Content-Type") 'text/plain'
					procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_config" "GET" "http://127.0.0.1:$iPort/__xs/reload_config"
					$bReloadConfigTextOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName repo_root reload_config" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTextTry++
			}

			if ( -not $bReloadConfigTextOK -and $iReloadConfigTextTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName repo_root reload_config : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_config_text_idle_after : timeout")
		} else {
			$iReloadConfigTry = 0
			$bReloadConfigOK = $false
			while ( $iReloadConfigTry -lt 10 ) {
				$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_config_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName repo_root reload_config_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName repo_root reload_config_json body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName repo_root reload_config_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Content-Type") 'application/json'
					procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_config_json" "GET" "http://127.0.0.1:$iPort/__xs/reload_config_json"
					$bReloadConfigOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName repo_root reload_config_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTry++
			}

			if ( -not $bReloadConfigOK -and $iReloadConfigTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName repo_root reload_config_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_config_idle_after : timeout")
		} else {
			$iReloadConfigTextTry = 0
			$bReloadConfigTextOK = $false
			while ( $iReloadConfigTextTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_config"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_config" $objResp.status 200 $objResp.body 'result=true'
					procCheckBodyContains $arrResults "$sExeName repo_root post_reload_config body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName repo_root post_reload_config type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config" "Content-Type") 'text/plain'
					procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload_config" "POST" "http://127.0.0.1:$iPort/__xs/reload_config"
					$bReloadConfigTextOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_config" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTextTry++
			}

			if ( -not $bReloadConfigTextOK -and $iReloadConfigTextTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName repo_root post_reload_config : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_config_text_post_idle_wait : timeout")
		} else {
			$iReloadConfigTry = 0
			$bReloadConfigOK = $false
			while ( $iReloadConfigTry -lt 10 ) {
				$objResp = procFetchEx "POST" "http://127.0.0.1:$iPort/__xs/reload_config_json"
				if ( $objResp.status -eq 200 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_config_json" $objResp.status 200 $objResp.body '"result":true'
					procCheckBodyContains $arrResults "$sExeName repo_root post_reload_config_json body" $objResp.body 'config reload queued'
					procCheckBodyContains $arrResults "$sExeName repo_root post_reload_config_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_config_json" "Content-Type") 'application/json'
					procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload_config_json" "POST" "http://127.0.0.1:$iPort/__xs/reload_config_json"
					$bReloadConfigOK = $true
					break
				}
				if ( $objResp.status -ne 409 ) {
					procCheck $arrResults "$sExeName repo_root post_reload_config_json" $objResp.status 200 $objResp.body
					break
				}
				Start-Sleep -Milliseconds 200
				$iReloadConfigTry++
			}

			if ( -not $bReloadConfigOK -and $iReloadConfigTry -ge 10 ) {
				$arrResults.Add("FAIL $sExeName repo_root post_reload_config_json : remained busy after retries")
			}
		}

		if ( -not (procWaitReloadIdle) ) {
			$arrResults.Add("FAIL $sExeName repo_root reload_config_post_idle_wait : timeout")
		}

		$objResp = procFetch "http://127.0.0.1:$iPort/json"
		procCheck $arrResults "$sExeName repo_root json_after_config_reload" $objResp.status 200 $objResp.body '"path":"/json"'

		if ( $bDebug ) {
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_root" "GET" "/__xs/bus" 404 'application/json' 'manage api not found'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_root" "HEAD" "/__xs/bus" 404 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_root" "POST" "/__xs/bus" 404 'application/json' 'manage api not found'

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName repo_root dashboard" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root dashboard body" $objResp.body 'http_req_count='
			procCheckBodyContains $arrResults "$sExeName repo_root dashboard type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root dashboard" "GET" "http://127.0.0.1:$iPort/__xs/dashboard"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName repo_root head_dashboard" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_dashboard type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_dashboard" "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard"
			procCheck $arrResults "$sExeName repo_root post_dashboard" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_dashboard allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_dashboard type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_dashboard" "POST" "http://127.0.0.1:$iPort/__xs/dashboard"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName repo_root dashboard_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root dashboard_json body" $objResp.body '"status"'
			procCheckBodyContains $arrResults "$sExeName repo_root dashboard_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root dashboard_json" "GET" "http://127.0.0.1:$iPort/__xs/dashboard_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName repo_root head_dashboard_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_dashboard_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_dashboard_json" "HEAD" "http://127.0.0.1:$iPort/__xs/dashboard_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard_json"
			procCheck $arrResults "$sExeName repo_root post_dashboard_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_dashboard_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_dashboard_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/dashboard_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_dashboard_json" "POST" "http://127.0.0.1:$iPort/__xs/dashboard_json"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/http_metrics"
			procCheck $arrResults "$sExeName repo_root http_metrics" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root http_metrics body" $objResp.body 'http_req_count='
			procCheckBodyContains $arrResults "$sExeName repo_root http_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root http_metrics" "GET" "http://127.0.0.1:$iPort/__xs/http_metrics"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics"
			procCheck $arrResults "$sExeName repo_root head_http_metrics" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_http_metrics type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_http_metrics" "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics"
			procCheck $arrResults "$sExeName repo_root post_http_metrics" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_http_metrics allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_http_metrics type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_http_metrics" "POST" "http://127.0.0.1:$iPort/__xs/http_metrics"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/http_metrics_json"
			procCheck $arrResults "$sExeName repo_root http_metrics_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root http_metrics_json body" $objResp.body '"http_req_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root http_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root http_metrics_json" "GET" "http://127.0.0.1:$iPort/__xs/http_metrics_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics_json"
			procCheck $arrResults "$sExeName repo_root head_http_metrics_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_http_metrics_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_http_metrics_json" "HEAD" "http://127.0.0.1:$iPort/__xs/http_metrics_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics_json"
			procCheck $arrResults "$sExeName repo_root post_http_metrics_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_http_metrics_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_http_metrics_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/http_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_http_metrics_json" "POST" "http://127.0.0.1:$iPort/__xs/http_metrics_json"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/ws_metrics"
			procCheck $arrResults "$sExeName repo_root ws_metrics" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root ws_metrics body" $objResp.body 'ws_open_count='
			procCheckBodyContains $arrResults "$sExeName repo_root ws_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root ws_metrics" "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics"
			procCheck $arrResults "$sExeName repo_root head_ws_metrics" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_ws_metrics type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_ws_metrics" "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics"
			procCheck $arrResults "$sExeName repo_root post_ws_metrics" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_ws_metrics allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_ws_metrics type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_ws_metrics" "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/ws_metrics_json"
			procCheck $arrResults "$sExeName repo_root ws_metrics_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root ws_metrics_json body" $objResp.body '"ws_open_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root ws_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root ws_metrics_json" "GET" "http://127.0.0.1:$iPort/__xs/ws_metrics_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics_json"
			procCheck $arrResults "$sExeName repo_root head_ws_metrics_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_ws_metrics_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_ws_metrics_json" "HEAD" "http://127.0.0.1:$iPort/__xs/ws_metrics_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics_json"
			procCheck $arrResults "$sExeName repo_root post_ws_metrics_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_ws_metrics_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_ws_metrics_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_ws_metrics_json" "POST" "http://127.0.0.1:$iPort/__xs/ws_metrics_json"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/xtp_metrics"
			procCheck $arrResults "$sExeName repo_root xtp_metrics" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root xtp_metrics body" $objResp.body 'xtp_open_count='
			procCheckBodyContains $arrResults "$sExeName repo_root xtp_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root xtp_metrics" "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics"
			procCheck $arrResults "$sExeName repo_root head_xtp_metrics" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_xtp_metrics type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_xtp_metrics" "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics"
			procCheck $arrResults "$sExeName repo_root post_xtp_metrics" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_xtp_metrics allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_xtp_metrics type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_xtp_metrics" "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"
			procCheck $arrResults "$sExeName repo_root xtp_metrics_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root xtp_metrics_json body" $objResp.body '"xtp_open_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root xtp_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root xtp_metrics_json" "GET" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"
			procCheck $arrResults "$sExeName repo_root head_xtp_metrics_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_xtp_metrics_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_xtp_metrics_json" "HEAD" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"
			procCheck $arrResults "$sExeName repo_root post_xtp_metrics_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_xtp_metrics_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_xtp_metrics_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_xtp_metrics_json" "POST" "http://127.0.0.1:$iPort/__xs/xtp_metrics_json"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/udp_metrics"
			procCheck $arrResults "$sExeName repo_root udp_metrics" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root udp_metrics body" $objResp.body 'udp_recv_count='
			procCheckBodyContains $arrResults "$sExeName repo_root udp_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root udp_metrics" "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics"
			procCheck $arrResults "$sExeName repo_root head_udp_metrics" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_udp_metrics type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_udp_metrics" "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics"
			procCheck $arrResults "$sExeName repo_root post_udp_metrics" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_udp_metrics allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_udp_metrics type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_udp_metrics" "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/udp_metrics_json"
			procCheck $arrResults "$sExeName repo_root udp_metrics_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root udp_metrics_json body" $objResp.body '"udp_recv_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root udp_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root udp_metrics_json" "GET" "http://127.0.0.1:$iPort/__xs/udp_metrics_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics_json"
			procCheck $arrResults "$sExeName repo_root head_udp_metrics_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_udp_metrics_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_udp_metrics_json" "HEAD" "http://127.0.0.1:$iPort/__xs/udp_metrics_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics_json"
			procCheck $arrResults "$sExeName repo_root post_udp_metrics_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_udp_metrics_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_udp_metrics_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_udp_metrics_json" "POST" "http://127.0.0.1:$iPort/__xs/udp_metrics_json"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/custom_metrics"
			procCheck $arrResults "$sExeName repo_root custom_metrics" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root custom_metrics body" $objResp.body 'custom_open_count='
			procCheckBodyContains $arrResults "$sExeName repo_root custom_metrics type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root custom_metrics" "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics"
			procCheck $arrResults "$sExeName repo_root head_custom_metrics" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_custom_metrics type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_custom_metrics" "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics"
			procCheck $arrResults "$sExeName repo_root post_custom_metrics" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_custom_metrics allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_custom_metrics type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_custom_metrics" "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/custom_metrics_json"
			procCheck $arrResults "$sExeName repo_root custom_metrics_json" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root custom_metrics_json body" $objResp.body '"custom_open_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root custom_metrics_json type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root custom_metrics_json" "GET" "http://127.0.0.1:$iPort/__xs/custom_metrics_json"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics_json"
			procCheck $arrResults "$sExeName repo_root head_custom_metrics_json" $iStatus 200 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_custom_metrics_json type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_custom_metrics_json" "HEAD" "http://127.0.0.1:$iPort/__xs/custom_metrics_json"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics_json"
			procCheck $arrResults "$sExeName repo_root post_custom_metrics_json" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_custom_metrics_json allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics_json") 'GET, HEAD'
			procCheckBodyContains $arrResults "$sExeName repo_root post_custom_metrics_json type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics_json" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_custom_metrics_json" "POST" "http://127.0.0.1:$iPort/__xs/custom_metrics_json"

			for ( $i = 0; $i -lt $arrMetricsClearText.Length; $i++ ) {
				$tblMetric = $arrMetricsClearText[$i]
				$objResp = procFetch ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName repo_root $($tblMetric.name)" $objResp.status 200 $objResp.body
				procCheckBodyContains $arrResults "$sExeName repo_root $($tblMetric.name) body" $objResp.body $tblMetric.token
				procCheckBodyContains $arrResults "$sExeName repo_root $($tblMetric.name) type" (procFetchHeaderMethod "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName repo_root $($tblMetric.name)" "GET" ("http://127.0.0.1:$iPort" + $tblMetric.path)

				$iStatus = procFetchStatusMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName repo_root head_$($tblMetric.name)" $iStatus 405 ""
				procCheckBodyContains $arrResults "$sExeName repo_root head_$($tblMetric.name) allow" (procFetchAllowMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)) 'GET'
				procCheckBodyContains $arrResults "$sExeName repo_root head_$($tblMetric.name) type" (procFetchHeaderMethod "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName repo_root head_$($tblMetric.name)" "HEAD" ("http://127.0.0.1:$iPort" + $tblMetric.path)

				$iStatus = procFetchStatusMethod "POST" ("http://127.0.0.1:$iPort" + $tblMetric.path)
				procCheck $arrResults "$sExeName repo_root post_$($tblMetric.name)" $iStatus 405 ""
				procCheckBodyContains $arrResults "$sExeName repo_root post_$($tblMetric.name) allow" (procFetchAllowMethod "POST" ("http://127.0.0.1:$iPort" + $tblMetric.path)) 'GET'
				procCheckBodyContains $arrResults "$sExeName repo_root post_$($tblMetric.name) type" (procFetchHeaderMethod "POST" ("http://127.0.0.1:$iPort" + $tblMetric.path) "Content-Type") 'text/plain'
				procCheckSecurityHeaders $arrResults "$sExeName repo_root post_$($tblMetric.name)" "POST" ("http://127.0.0.1:$iPort" + $tblMetric.path)
			}

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName repo_root reload_clear" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root reload_clear body" $objResp.body 'reload_total_count='
			procCheckBodyContains $arrResults "$sExeName repo_root reload_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_clear" "GET" "http://127.0.0.1:$iPort/__xs/reload_clear"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName repo_root reload_reset" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root reload_reset body" $objResp.body 'reload_total_count='
			procCheckBodyContains $arrResults "$sExeName repo_root reload_reset type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root reload_reset" "GET" "http://127.0.0.1:$iPort/__xs/reload_reset"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName repo_root check_config_clear" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root check_config_clear body" $objResp.body 'check_total_count='
			procCheckBodyContains $arrResults "$sExeName repo_root check_config_clear type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root check_config_clear" "GET" "http://127.0.0.1:$iPort/__xs/check_config_clear"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName repo_root head_reload_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_reload_clear allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_reload_clear type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_reload_clear" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_clear"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName repo_root head_reload_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_reload_reset allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_reload_reset type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_reload_reset" "HEAD" "http://127.0.0.1:$iPort/__xs/reload_reset"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName repo_root head_check_config_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_check_config_clear allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_check_config_clear type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_check_config_clear" "HEAD" "http://127.0.0.1:$iPort/__xs/check_config_clear"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_clear"
			procCheck $arrResults "$sExeName repo_root post_reload_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_reload_clear allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_reload_clear type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload_clear" "POST" "http://127.0.0.1:$iPort/__xs/reload_clear"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_reset"
			procCheck $arrResults "$sExeName repo_root post_reload_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_reload_reset allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_reload_reset type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/reload_reset" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_reload_reset" "POST" "http://127.0.0.1:$iPort/__xs/reload_reset"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config_clear"
			procCheck $arrResults "$sExeName repo_root post_check_config_clear" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_check_config_clear allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config_clear") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_check_config_clear type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/check_config_clear" "Content-Type") 'text/plain'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_check_config_clear" "POST" "http://127.0.0.1:$iPort/__xs/check_config_clear"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName repo_root bus_status" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_status body" $objResp.body '"data_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_status type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_status" "GET" "http://127.0.0.1:$iPort/__xs/bus/status"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName repo_root head_bus_status" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_status allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_status type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_status" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/status"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status"
			procCheck $arrResults "$sExeName repo_root post_bus_status" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_status allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_status type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/status" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_status" "POST" "http://127.0.0.1:$iPort/__xs/bus/status"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName repo_root bus_namespaces" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_namespaces body" $objResp.body '"namespace_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_namespaces type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_namespaces" "GET" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName repo_root head_bus_namespaces" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_namespaces allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_namespaces type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_namespaces" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"
			procCheck $arrResults "$sExeName repo_root post_bus_namespaces" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_namespaces allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_namespaces type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_namespaces" "POST" "http://127.0.0.1:$iPort/__xs/bus/namespaces"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName repo_root bus_registry" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_registry body" $objResp.body '"items"'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_registry type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_registry" "GET" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName repo_root head_bus_registry" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_registry allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_registry type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_registry" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"
			procCheck $arrResults "$sExeName repo_root post_bus_registry" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_registry allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_registry type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/registry" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_registry" "POST" "http://127.0.0.1:$iPort/__xs/bus/registry"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName repo_root bus_limits" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_limits body" $objResp.body '"data_limit"'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_limits type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_limits" "GET" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName repo_root head_bus_limits" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_limits allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_limits type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_limits" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"
			procCheck $arrResults "$sExeName repo_root post_bus_limits" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_limits allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_limits type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/limits" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_limits" "POST" "http://127.0.0.1:$iPort/__xs/bus/limits"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName repo_root bus_send" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_send body" $objResp.body '"result":true'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_send type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_send" "GET" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName repo_root head_bus_send" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_send allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_send type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_send" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"
			procCheck $arrResults "$sExeName repo_root post_bus_send" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_send allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_send type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_send" "POST" "http://127.0.0.1:$iPort/__xs/bus/send?topic=stable.smoke&text=hello"

			$objResp = procFetch "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName repo_root bus_reset" $objResp.status 200 $objResp.body
			procCheckBodyContains $arrResults "$sExeName repo_root bus_reset body" $objResp.body '"sweep_count"'
			procCheckBodyContains $arrResults "$sExeName repo_root bus_reset type" (procFetchHeaderMethod "GET" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root bus_reset" "GET" "http://127.0.0.1:$iPort/__xs/bus/reset"

			$iStatus = procFetchStatusMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName repo_root head_bus_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_reset allow" (procFetchAllowMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root head_bus_reset type" (procFetchHeaderMethod "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root head_bus_reset" "HEAD" "http://127.0.0.1:$iPort/__xs/bus/reset"

			$iStatus = procFetchStatusMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"
			procCheck $arrResults "$sExeName repo_root post_bus_reset" $iStatus 405 ""
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_reset allow" (procFetchAllowMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset") 'GET'
			procCheckBodyContains $arrResults "$sExeName repo_root post_bus_reset type" (procFetchHeaderMethod "POST" "http://127.0.0.1:$iPort/__xs/bus/reset" "Content-Type") 'application/json'
			procCheckSecurityHeaders $arrResults "$sExeName repo_root post_bus_reset" "POST" "http://127.0.0.1:$iPort/__xs/bus/reset"
		} else {
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_root" "GET" "/__xs/bus" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_root_disabled" "HEAD" "/__xs/bus" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_root_disabled" "POST" "/__xs/bus" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root dashboard" "GET" "/__xs/dashboard" 403 'text/plain' 'dashboard api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_dashboard_disabled" "HEAD" "/__xs/dashboard" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_dashboard_disabled" "POST" "/__xs/dashboard" 403 'text/plain' 'dashboard api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root dashboard_json" "GET" "/__xs/dashboard_json" 403 'application/json' 'dashboard json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_dashboard_json_disabled" "HEAD" "/__xs/dashboard_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_dashboard_json_disabled" "POST" "/__xs/dashboard_json" 403 'application/json' 'dashboard json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root http_metrics" "GET" "/__xs/http_metrics" 403 'text/plain' 'http metrics api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_http_metrics_disabled" "HEAD" "/__xs/http_metrics" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_http_metrics_disabled" "POST" "/__xs/http_metrics" 403 'text/plain' 'http metrics api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root http_metrics_json" "GET" "/__xs/http_metrics_json" 403 'application/json' 'http metrics json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_http_metrics_json_disabled" "HEAD" "/__xs/http_metrics_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_http_metrics_json_disabled" "POST" "/__xs/http_metrics_json" 403 'application/json' 'http metrics json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root ws_metrics" "GET" "/__xs/ws_metrics" 403 'text/plain' 'ws metrics api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_ws_metrics_disabled" "HEAD" "/__xs/ws_metrics" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_ws_metrics_disabled" "POST" "/__xs/ws_metrics" 403 'text/plain' 'ws metrics api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root ws_metrics_json" "GET" "/__xs/ws_metrics_json" 403 'application/json' 'ws metrics json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_ws_metrics_json_disabled" "HEAD" "/__xs/ws_metrics_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_ws_metrics_json_disabled" "POST" "/__xs/ws_metrics_json" 403 'application/json' 'ws metrics json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root xtp_metrics" "GET" "/__xs/xtp_metrics" 403 'text/plain' 'xtp metrics api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_xtp_metrics_disabled" "HEAD" "/__xs/xtp_metrics" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_xtp_metrics_disabled" "POST" "/__xs/xtp_metrics" 403 'text/plain' 'xtp metrics api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root xtp_metrics_json" "GET" "/__xs/xtp_metrics_json" 403 'application/json' 'xtp metrics json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_xtp_metrics_json_disabled" "HEAD" "/__xs/xtp_metrics_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_xtp_metrics_json_disabled" "POST" "/__xs/xtp_metrics_json" 403 'application/json' 'xtp metrics json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root udp_metrics" "GET" "/__xs/udp_metrics" 403 'text/plain' 'udp metrics api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_udp_metrics_disabled" "HEAD" "/__xs/udp_metrics" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_udp_metrics_disabled" "POST" "/__xs/udp_metrics" 403 'text/plain' 'udp metrics api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root udp_metrics_json" "GET" "/__xs/udp_metrics_json" 403 'application/json' 'udp metrics json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_udp_metrics_json_disabled" "HEAD" "/__xs/udp_metrics_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_udp_metrics_json_disabled" "POST" "/__xs/udp_metrics_json" 403 'application/json' 'udp metrics json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root custom_metrics" "GET" "/__xs/custom_metrics" 403 'text/plain' 'custom metrics api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_custom_metrics_disabled" "HEAD" "/__xs/custom_metrics" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_custom_metrics_disabled" "POST" "/__xs/custom_metrics" 403 'text/plain' 'custom metrics api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root custom_metrics_json" "GET" "/__xs/custom_metrics_json" 403 'application/json' 'custom metrics json api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_custom_metrics_json_disabled" "HEAD" "/__xs/custom_metrics_json" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_custom_metrics_json_disabled" "POST" "/__xs/custom_metrics_json" 403 'application/json' 'custom metrics json api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root http_metrics_clear" "GET" "/__xs/http_metrics_clear" 403 'text/plain' 'http metrics clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_http_metrics_clear_disabled" "HEAD" "/__xs/http_metrics_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_http_metrics_clear_disabled" "POST" "/__xs/http_metrics_clear" 403 'text/plain' 'http metrics clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root ws_metrics_clear" "GET" "/__xs/ws_metrics_clear" 403 'text/plain' 'ws metrics clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_ws_metrics_clear_disabled" "HEAD" "/__xs/ws_metrics_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_ws_metrics_clear_disabled" "POST" "/__xs/ws_metrics_clear" 403 'text/plain' 'ws metrics clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root xtp_metrics_clear" "GET" "/__xs/xtp_metrics_clear" 403 'text/plain' 'xtp metrics clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_xtp_metrics_clear_disabled" "HEAD" "/__xs/xtp_metrics_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_xtp_metrics_clear_disabled" "POST" "/__xs/xtp_metrics_clear" 403 'text/plain' 'xtp metrics clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root udp_metrics_clear" "GET" "/__xs/udp_metrics_clear" 403 'text/plain' 'udp metrics clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_udp_metrics_clear_disabled" "HEAD" "/__xs/udp_metrics_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_udp_metrics_clear_disabled" "POST" "/__xs/udp_metrics_clear" 403 'text/plain' 'udp metrics clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root custom_metrics_clear" "GET" "/__xs/custom_metrics_clear" 403 'text/plain' 'custom metrics clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_custom_metrics_clear_disabled" "HEAD" "/__xs/custom_metrics_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_custom_metrics_clear_disabled" "POST" "/__xs/custom_metrics_clear" 403 'text/plain' 'custom metrics clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root reload_clear" "GET" "/__xs/reload_clear" 403 'text/plain' 'config reload clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_reload_clear_disabled" "HEAD" "/__xs/reload_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_reload_clear_disabled" "POST" "/__xs/reload_clear" 403 'text/plain' 'config reload clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root reload_reset" "GET" "/__xs/reload_reset" 403 'text/plain' 'config reload reset api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_reload_reset_disabled" "HEAD" "/__xs/reload_reset" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_reload_reset_disabled" "POST" "/__xs/reload_reset" 403 'text/plain' 'config reload reset api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root check_config_clear" "GET" "/__xs/check_config_clear" 403 'text/plain' 'check config clear api only available in xsdbg'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_check_config_clear_disabled" "HEAD" "/__xs/check_config_clear" 403 'text/plain' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_check_config_clear_disabled" "POST" "/__xs/check_config_clear" 403 'text/plain' 'check config clear api only available in xsdbg'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_status" "GET" "/__xs/bus/status" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_status_disabled" "HEAD" "/__xs/bus/status" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_status_disabled" "POST" "/__xs/bus/status" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_namespaces" "GET" "/__xs/bus/namespaces" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_namespaces_disabled" "HEAD" "/__xs/bus/namespaces" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_namespaces_disabled" "POST" "/__xs/bus/namespaces" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_registry" "GET" "/__xs/bus/registry" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_registry_disabled" "HEAD" "/__xs/bus/registry" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_registry_disabled" "POST" "/__xs/bus/registry" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_limits" "GET" "/__xs/bus/limits" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_limits_disabled" "HEAD" "/__xs/bus/limits" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_limits_disabled" "POST" "/__xs/bus/limits" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_send" "GET" "/__xs/bus/send?topic=stable.smoke&text=hello" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_send_disabled" "HEAD" "/__xs/bus/send?topic=stable.smoke&text=hello" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_send_disabled" "POST" "/__xs/bus/send?topic=stable.smoke&text=hello" 403 'application/json' 'bus api not included in production xs'

			procCheckDisabledEndpoint $arrResults $sExeName "repo_root bus_reset" "GET" "/__xs/bus/reset" 403 'application/json' 'bus api not included in production xs'
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root head_bus_reset_disabled" "HEAD" "/__xs/bus/reset" 403 'application/json' $null
			procCheckDisabledEndpoint $arrResults $sExeName "repo_root post_bus_reset_disabled" "POST" "/__xs/bus/reset" 403 'application/json' 'bus api not included in production xs'
		}

		$iCode = 0
		foreach ( $sLine in $arrResults ) {
			if ( $sLine.StartsWith("FAIL ") ) {
				$iCode = 1
				break
			}
		}

		return @{ code = $iCode; lines = $arrResults }
	} finally {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		Start-Sleep -Milliseconds 500
	}
}

function procRunXtpCase([string]$sExeName, [bool]$bDebug)
{
	$sExePath = Join-Path $sReleaseDir $sExeName
	$arrResults = New-Object 'System.Collections.Generic.List[string]'
	$sClientExe = $null

	if ( -not (Test-Path $sExePath) ) {
		$arrResults.Add("FAIL $sExeName xtp : executable not found")
		return @{
			code = 1
			lines = $arrResults
		}
	}

	try {
		$sClientExe = procBuildXtpSmokeClient
	} catch {
		$arrResults.Add("FAIL $sExeName xtp_build : $($_.Exception.Message)")
		return @{
			code = 1
			lines = $arrResults
		}
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList "xs_manage_xtp_test.json" -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden

	try {
		if ( -not (procWaitReady) ) {
			$arrResults.Add("FAIL $sExeName xtp_ready : timeout")
			return @{
				code = 1
				lines = $arrResults
			}
		}

		Start-Sleep -Milliseconds 500

		$tblRun = procRunClientRetry $sClientExe @("127.0.0.1", "$iXtpPort", "demo.callself", "tag=smoke")
		if ( $tblRun.code -ne 0 ) {
			$arrResults.Add("FAIL $sExeName xtp_callself : client exit $($tblRun.code)")
		} elseif ( ($tblRun.text -like "*status=0*") -and ($tblRun.text -like "*cmd=xtp.reply*") -and ($tblRun.text -like "*self call ok*") ) {
			$arrResults.Add("OK   $sExeName xtp_callself : 0")
		} else {
			$arrResults.Add("FAIL $sExeName xtp_callself : unexpected body")
		}

		$tblRun = procRunClientRetry $sClientExe @("127.0.0.1", "$iXtpPort", "demo.callrequeststatus", "port=9096", "inner_cmd=demo.ping", "tag=smoke")
		if ( $tblRun.code -ne 0 ) {
			$arrResults.Add("FAIL $sExeName xtp_callrequeststatus : client exit $($tblRun.code)")
		} elseif ( ($tblRun.text -like "*status=0*") -and ($tblRun.text -like "*cmd=xtp.reply*") -and ($tblRun.text -like "*request status ok=true*") ) {
			$arrResults.Add("OK   $sExeName xtp_callrequeststatus : 0")
		} else {
			$arrResults.Add("FAIL $sExeName xtp_callrequeststatus : unexpected body")
		}

		$iCode = 0
		foreach ( $sLine in $arrResults ) {
			if ( $sLine.StartsWith("FAIL ") ) {
				$iCode = 1
				break
			}
		}

		return @{
			code = $iCode
			lines = $arrResults
		}
	} finally {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		Start-Sleep -Milliseconds 500
	}
}

function procRunWsCase([string]$sExeName)
{
	$sExePath = Join-Path $sReleaseDir $sExeName
	$arrResults = New-Object 'System.Collections.Generic.List[string]'
	$sClientExe = $null

	if ( -not (Test-Path $sExePath) ) {
		$arrResults.Add("FAIL $sExeName ws : executable not found")
		return @{ code = 1; lines = $arrResults }
	}

	try {
		if ( $script:sWsClientExe -and (Test-Path $script:sWsClientExe) ) {
			$sClientExe = $script:sWsClientExe
		} else {
			$script:sWsClientExe = procBuildCClient "ws_smoke_client.c" "ws_smoke_client.exe"
			$sClientExe = $script:sWsClientExe
		}
	} catch {
		$arrResults.Add("FAIL $sExeName ws_build : $($_.Exception.Message)")
		return @{ code = 1; lines = $arrResults }
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList "xs_manage_ws_test.json" -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden

	try {
		if ( -not (procWaitReady) ) {
			$arrResults.Add("FAIL $sExeName ws_ready : timeout")
			return @{ code = 1; lines = $arrResults }
		}

		Start-Sleep -Milliseconds 500
		$tblRun = procRunClientRetry $sClientExe @("127.0.0.1", "8081", "smoke ws")
		if ( $tblRun.code -ne 0 ) {
			$arrResults.Add("FAIL $sExeName ws_echo : client exit $($tblRun.code)")
		} elseif ( ($tblRun.text -like "*ws demo*") -and ($tblRun.text -like "*text=smoke ws*") -and ($tblRun.text -like "*protocol=xs-demo*") ) {
			$arrResults.Add("OK   $sExeName ws_echo : 0")
		} else {
			$arrResults.Add("FAIL $sExeName ws_echo : unexpected body")
		}

		$iCode = 0
		foreach ( $sLine in $arrResults ) {
			if ( $sLine.StartsWith("FAIL ") ) {
				$iCode = 1
				break
			}
		}

		return @{ code = $iCode; lines = $arrResults }
	} finally {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		Start-Sleep -Milliseconds 500
	}
}

function procRunCustomCase([string]$sExeName)
{
	$sExePath = Join-Path $sReleaseDir $sExeName
	$arrResults = New-Object 'System.Collections.Generic.List[string]'
	$sClientExe = $null

	if ( -not (Test-Path $sExePath) ) {
		$arrResults.Add("FAIL $sExeName custom : executable not found")
		return @{ code = 1; lines = $arrResults }
	}

	try {
		if ( $script:sCustomClientExe -and (Test-Path $script:sCustomClientExe) ) {
			$sClientExe = $script:sCustomClientExe
		} else {
			$script:sCustomClientExe = procBuildCClient "custom_smoke_client.c" "custom_smoke_client.exe"
			$sClientExe = $script:sCustomClientExe
		}
	} catch {
		$arrResults.Add("FAIL $sExeName custom_build : $($_.Exception.Message)")
		return @{ code = 1; lines = $arrResults }
	}

	$objProc = Start-Process -FilePath $sExePath -ArgumentList "xs_manage_custom_test.json" -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden

	try {
		if ( -not (procWaitReady) ) {
			$arrResults.Add("FAIL $sExeName custom_ready : timeout")
			return @{ code = 1; lines = $arrResults }
		}

		Start-Sleep -Milliseconds 500
		$tblRun = procRunClientRetry $sClientExe @("127.0.0.1", "9098", "smoke custom")
		if ( $tblRun.code -ne 0 ) {
			$arrResults.Add("FAIL $sExeName custom_echo : client exit $($tblRun.code)")
		} elseif ( ($tblRun.text -like "*custom demo*") -and ($tblRun.text -like "*data=smoke custom*") ) {
			$arrResults.Add("OK   $sExeName custom_echo : 0")
		} else {
			$arrResults.Add("FAIL $sExeName custom_echo : unexpected body")
		}

		$iCode = 0
		foreach ( $sLine in $arrResults ) {
			if ( $sLine.StartsWith("FAIL ") ) {
				$iCode = 1
				break
			}
		}

		return @{ code = $iCode; lines = $arrResults }
	} finally {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
		Start-Sleep -Milliseconds 500
	}
}

procCleanupGeneratedFiles

$arrOut = New-Object 'System.Collections.Generic.List[string]'
$iExit = 0
$sRootMemReportHashBefore = procGetFileHashText $script:sRootMemReport
$sReleaseMemReportHashBefore = procGetFileHashText $script:sReleaseMemReport

$tblCase = procRunStaticHomepageAudit
$arrOut.Add("[static-homepage]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunCase "xs.exe" $false
$arrOut.Add("[xs]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunRepoRootCase "xs.exe" $false
$arrOut.Add("[xs-root]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunXtpCase "xs.exe" $false
$arrOut.Add("[xs-xtp]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunWsCase "xs.exe"
$arrOut.Add("[xs-ws]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunCustomCase "xs.exe"
$arrOut.Add("[xs-custom]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunCase "xsdbg.exe" $true
$arrOut.Add("[xsdbg]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunRepoRootCase "xsdbg.exe" $true
$arrOut.Add("[xsdbg-root]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunXtpCase "xsdbg.exe" $true
$arrOut.Add("[xsdbg-xtp]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunWsCase "xsdbg.exe"
$arrOut.Add("[xsdbg-ws]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

$tblCase = procRunCustomCase "xsdbg.exe"
$arrOut.Add("[xsdbg-custom]")
procAppendLines $arrOut $tblCase.lines
if ( $tblCase.code -ne 0 ) {
	$iExit = $tblCase.code
}

foreach ( $sPath in @($script:sXtpClientExe, $script:sWsClientExe, $script:sCustomClientExe) ) {
	if ( $sPath -and (Test-Path $sPath) ) {
		Remove-Item $sPath -Force -ErrorAction SilentlyContinue
	}
}

procCleanupGeneratedFiles

if ( (procAppendArtifactCleanupCheck $arrOut $sRootMemReportHashBefore $sReleaseMemReportHashBefore) -ne 0 ) {
	$iExit = 1
}

if ( (procAppendProcessCleanupCheck $arrOut) -ne 0 ) {
	$iExit = 1
}

$arrOut | ForEach-Object { Write-Output $_ }

exit $iExit
