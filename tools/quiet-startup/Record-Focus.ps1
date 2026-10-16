param([ValidateRange(5,180)][int]$Seconds = 90, [switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$profilePath = Join-Path $PSScriptRoot 'focus.wprp'
[xml]$profileXml = Get-Content -LiteralPath $profilePath -Raw
if ($profileXml.WindowsPerformanceRecorder.Profiles.EventProvider.Name -ne '8c416c79-d49b-4f01-a467-e56d3aa8234c' -or
    $profileXml.WindowsPerformanceRecorder.Profiles.EventProvider.Keywords.Keyword.Value -ne '0x22000') {
    throw 'Unexpected focus provider profile.'
}
$wpr = (Get-Command wpr.exe -ErrorAction Stop).Source
if ($CheckOnly) {
    @{profile=$profilePath; seconds=$Seconds; wpr=$wpr; recording_started=$false} | ConvertTo-Json -Compress
    exit 0
}
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Windows requires an administrator PowerShell for this WPR capture. No recording was started.'
}
$sessionName = 'GearsVRFocus-' + [Guid]::NewGuid().ToString('N')
$directory = Join-Path $root ('analysis/runtime/focus-etw-' + $sessionName)
[void][IO.Directory]::CreateDirectory($directory)
$output = Join-Path $directory 'focus.etl'
$stopFile = Join-Path $directory 'stop'
$statusPath = Join-Path $directory 'status.json'
$status = @{instance=$sessionName; directory=$directory; etl=$output; stop_file=$stopFile; phase='starting'; seconds=$Seconds; owner_pid=$PID; profile_sha256=(Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash}
function Save-Status {
    $json = $status | ConvertTo-Json -Depth 4
    [IO.File]::WriteAllText($statusPath, $json)
    [IO.File]::WriteAllText((Join-Path $root 'analysis/runtime/focus-etw-current.json'), $json)
}
$started = $false
try {
    Save-Status
    & $wpr -start ($profilePath + '!QuietFocus') -instancename $sessionName *> (Join-Path $directory 'start.log')
    if ($LASTEXITCODE -ne 0) { throw ('WPR start failed; see ' + (Join-Path $directory 'start.log')) }
    $started = $true
    $status.phase = 'recording'
    Save-Status
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while ($timer.Elapsed.TotalSeconds -lt $Seconds -and -not (Test-Path -LiteralPath $stopFile)) {
        Start-Sleep -Milliseconds 200
    }
} catch {
    $status.error = $_.Exception.Message
    $status.phase = 'failed'
    throw
} finally {
    if ($started) {
        & $wpr -stop $output -skipPdbGen -instancename $sessionName *> (Join-Path $directory 'stop.log')
        $status.stop_exit_code = $LASTEXITCODE
        if ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $output)) {
            $status.phase = 'saved'
        } else {
            $status.phase = 'stop_failed'
            # Cancel only the unique instance created by this invocation.
            & $wpr -cancel -instancename $sessionName *> (Join-Path $directory 'cancel.log')
            $status.cancel_exit_code = $LASTEXITCODE
        }
    }
    Save-Status
}
$status | ConvertTo-Json -Depth 4
if ($status.phase -ne 'saved') { exit 1 }
