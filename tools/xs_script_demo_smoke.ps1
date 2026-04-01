param(
	[string]$sConfig = "xs_script_demo.json",
	[int]$iPort = 18081
)

$sRepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$sReleaseDir = Join-Path $sRepoRoot "release"
$sExePath = Join-Path $sReleaseDir "xs.exe"

function procFetch([string]$sUrl)
{
	$iStatus = 0
	$sBody = ""
	$sBodyFile = [System.IO.Path]::GetTempFileName()
	$arrArgs = @("-s", "--max-time", "2", "-o", $sBodyFile, "-w", "%{http_code}", $sUrl)

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

function procWaitReady([string]$sUrl)
{
	$i = 0

	while ( $i -lt 50 ) {
		$objResp = procFetch $sUrl
		if ( $objResp.status -eq 200 ) {
			return $true
		}
		Start-Sleep -Milliseconds 200
		$i++
	}

	return $false
}

function procCheckBody([string]$sName, $objResp, [string]$sExpect)
{
	if ( $objResp.status -ne 200 ) {
		throw "$sName status=$($objResp.status)"
	}
	if ( $objResp.body -notlike "*$sExpect*" ) {
		throw "$sName missing=$sExpect"
	}
}

if ( -not (Test-Path $sExePath) ) {
	Write-Error "xs.exe not found: $sExePath"
	exit 1
}

Get-Process xs -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

$objProc = $null

try {
	$objProc = Start-Process -FilePath $sExePath -ArgumentList $sConfig -WorkingDirectory $sReleaseDir -PassThru -WindowStyle Hidden
	if ( -not (procWaitReady "http://127.0.0.1:$iPort/test") ) {
		throw "server not ready"
	}

	$objTest = procFetch "http://127.0.0.1:$iPort/test"
	$objTemplate = procFetch "http://127.0.0.1:$iPort/template"
	$objChart = procFetch "http://127.0.0.1:$iPort/chart/get"

	procCheckBody "test" $objTest "page load success"
	procCheckBody "template" $objTemplate "<html"
	procCheckBody "chart" $objChart '"series"'

	Write-Output "OK   script_demo test"
	Write-Output "OK   script_demo template"
	Write-Output "OK   script_demo chart"
	exit 0
} catch {
	Write-Error $_
	exit 1
} finally {
	if ( $objProc -and (-not $objProc.HasExited) ) {
		Stop-Process -Id $objProc.Id -Force -ErrorAction SilentlyContinue
	}
	Get-Process xs -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}
