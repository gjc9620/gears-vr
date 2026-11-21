param([switch]$DirectLevel, [switch]$PauseOnUnfocus, [switch]$Foreground,
      [switch]$DisableBackgroundInput = $true, [string]$GamePath, [switch]$PythonCheckOnly, [string]$LaunchReceipt,
      [switch]$ExperimentalPreserveUpscaler, [switch]$SteamClientLaunch)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$OutputEncoding = [Console]::OutputEncoding
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
function Find-GearsVrPython([string]$Root) {
    if ($null -eq $script:PythonProbeResults) { $script:PythonProbeResults=@() }
    $preferred = [Environment]::GetEnvironmentVariable('GEARSVR_PYTHON')
    $explicitPython = -not [string]::IsNullOrWhiteSpace($preferred)
    $candidates = if ($explicitPython) {
        @([Environment]::ExpandEnvironmentVariables($preferred.Trim().Trim('"')))
    } else {
        @((Join-Path $Root 'runtime/python/python.exe'),
          (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'))
    }
    foreach ($candidate in $candidates) {
        $detail = [ordered]@{candidate=$candidate; status='missing'; timeout=$false}
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $script:PythonProbeResults += $detail
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $probe = [Diagnostics.Process]::new()
        try {
            $probe.StartInfo = [Diagnostics.ProcessStartInfo]::new()
            $probe.StartInfo.FileName = $candidate
            $probe.StartInfo.Arguments = '-c "import sys; print(sys.executable); print(sys.version_info[0])"'
            $probe.StartInfo.UseShellExecute = $false
            $probe.StartInfo.CreateNoWindow = $true
            $probe.StartInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
            $probe.StartInfo.RedirectStandardOutput = $true
            $probe.StartInfo.RedirectStandardError = $true
            $probe.StartInfo.StandardOutputEncoding = [Text.Encoding]::UTF8
            $probe.StartInfo.StandardErrorEncoding = [Text.Encoding]::UTF8
            $probe.StartInfo.EnvironmentVariables['PYTHONIOENCODING'] = 'utf-8'
            [void]$probe.Start()
            $detail.probe_pid=$probe.Id; $detail.wait_limit_ms=5000
            $output = $probe.StandardOutput.ReadToEndAsync()
            $errors = $probe.StandardError.ReadToEndAsync()
            if (-not $probe.WaitForExit(5000)) {
                $detail.status='timeout'; $detail.timeout=$true; $detail.elapsed_ms=$timer.ElapsedMilliseconds
                $probe.Kill() # Only this disposable interpreter locator probe.
                continue
            }
            $detail.exit_code=$probe.ExitCode; $detail.stdout=$output.Result.Substring(0,[Math]::Min(4096,$output.Result.Length)); $detail.stderr=$errors.Result.Substring(0,[Math]::Min(4096,$errors.Result.Length))
            $detail.elapsed_ms=$timer.ElapsedMilliseconds; $detail.status='invalid_output'
            $lines = $output.Result.Trim() -split '[\r\n]+'
            if ($probe.ExitCode -eq 0 -and $lines.Count -eq 2 -and $lines[1] -eq '3' -and
                (Test-Path -LiteralPath $lines[0] -PathType Leaf)) {
                $detail.status='accepted'; return [IO.Path]::GetFullPath($lines[0])
            }
        } catch {
            $detail.status='probe_error'; $detail.error=$_.Exception.Message; $detail.elapsed_ms=$timer.ElapsedMilliseconds
            # Explicit selection never falls through to a different interpreter.
        } finally { $probe.Dispose() }
    }
    if ($explicitPython) { throw 'GEARSVR_PYTHON is not a usable Python 3 executable; no game startup actions were taken.' }
    throw 'No usable Python 3 found in runtime/python or the development cache; set GEARSVR_PYTHON explicitly.'
}
$script:PythonProbeResults=@()
try { $python = Find-GearsVrPython $workspace } catch {
    $rejection = @{schema=1;kind='startup_prelaunch_rejection';stage='python_discovery';game_creation_attempted=$false;injection_attempted=$false;prime_attempted=$false;probes=@($script:PythonProbeResults)}
    if ($LaunchReceipt) {
        $rejection.launch_receipt=[IO.Path]::GetFullPath($LaunchReceipt)
        $hasher=[Security.Cryptography.SHA256]::Create()
        try { $rejection.startup_script_sha256=([BitConverter]::ToString($hasher.ComputeHash([IO.File]::ReadAllBytes($PSCommandPath)))).Replace('-','').ToLowerInvariant() } finally { $hasher.Dispose() }
        $target=$rejection.launch_receipt+'.no-start.json'; $pending=$target+'.'+[Guid]::NewGuid().ToString('N')+'.tmp'
        try {
            [IO.File]::WriteAllText($pending,($rejection | ConvertTo-Json -Depth 6 -Compress),(New-Object Text.UTF8Encoding($false)))
            [IO.File]::Move($pending,$target)
        } catch { [Console]::Error.WriteLine('Prelaunch rejection receipt could not be published: '+$_.Exception.Message) }
    }
    [Console]::Error.WriteLine(($rejection | ConvertTo-Json -Depth 6 -Compress))
    throw
}
$env:GEARSVR_PYTHON = $python
if ($PythonCheckOnly) {
    @{python=$python; check_only=$true} | ConvertTo-Json -Compress
    exit 0
}
$game = Join-Path $workspace 'Gears of War Reloaded'
$exe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
if ($GamePath) { $exe = $GamePath }
$validated = & $python (Join-Path $PSScriptRoot 'game_install.py') --game-path $exe
if ($LASTEXITCODE -ne 0) { throw 'Unsupported game installation; no startup changes made.' }
$exe = [string]$validated
$defaultExe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
# A windowless helper can remain after the main game exits. Do not count it as
# a playable instance; still refuse a second visible game, including a hung one.
$isolatedInstance = -not [string]::IsNullOrEmpty($env:GEARSVR_INSTANCE_ROOT)
# Private performance comparison: only the existing isolated, observe-only path.
$privateParallelMode = $env:GEARSVR_TEST_PARALLEL_RENDERING_MODE
if (-not [string]::IsNullOrEmpty($privateParallelMode)) {
    if ($privateParallelMode -cnotin @('0', '1') -or -not $isolatedInstance -or $Foreground -or
        -not $DisableBackgroundInput -or $env:GEARSVR_TEST_WINDOWS_BEHIND -cne '1' -or
        $env:GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY -cne '1' -or $env:GEARSVR_TEST_MUTE_GAME -cne '1') {
        throw 'Rendering-mode comparison requires an explicit 0/1 and the protected isolated background path.'
    }
}
if ($isolatedInstance) {
    & $python (Join-Path $PSScriptRoot 'instance_policy.py')
    if ($LASTEXITCODE -ne 0) { throw 'Invalid isolated instance; no game startup actions taken.' }
}
& $python (Join-Path $PSScriptRoot 'instance_policy.py') --check-startup
if ($LASTEXITCODE -ne 0) { throw 'A shared-profile game is already active; use the isolated multi-instance launcher.' }
# Preserve the default reversible OFF policy. The explicit SR experiment only
# checks its fixed profile and leaves the player's upscaler settings unchanged.
# Other installs require DLSS already OFF unless that experiment is selected.
$isDefault = [IO.Path]::GetFullPath($exe) -eq [IO.Path]::GetFullPath($defaultExe)
$graphicsAction = if ($ExperimentalPreserveUpscaler) { 'check-experimental-sr' } elseif ($isDefault) { 'apply' } else { 'check' }
& $python (Join-Path $PSScriptRoot 'graphics_policy.py') $graphicsAction --game-path $exe
if ($LASTEXITCODE -ne 0) { throw 'Upscaler startup policy failed.' }
# Custom installations are read-only: validation above accepts original or known
# patched config; never apply repository backups to another installation.
if ($isDefault -and -not $isolatedInstance) {
    & $python (Join-Path $PSScriptRoot 'configure_startup.py') apply
    if ($LASTEXITCODE -ne 0) { throw 'Startup config validation failed.' }
}
$launchArguments = @()
if (-not [string]::IsNullOrEmpty($privateParallelMode)) { $launchArguments += "-PARALLELRENDERING=$privateParallelMode" }
if (-not $PauseOnUnfocus) { $launchArguments += '-NoPauseUnfocus' }
if ($isolatedInstance -and $env:GEARSVR_SKIP_STARTUP_MOVIES -eq '1') {
    # Explicit private multi-instance startup option. Shared/manual launches
    # retain their original startup and cinematic policy.
    $launchArguments += '-nomoviestartup'
}
if ($DirectLevel) {
    # Experimental UE3 map URL; does not request resume of an existing checkpoint.
    $launchArguments = @('SP_Prison_P') + $launchArguments
}
$launchCommand = @((Join-Path $PSScriptRoot 'background_launch.py'), '--game-path', $exe)
foreach ($argument in $launchArguments) { $launchCommand += "--game-arg=$argument" }
if ($Foreground) { $launchCommand += '--foreground' }
if ($SteamClientLaunch) { $launchCommand += '--steam-client-launch' }
if ($DisableBackgroundInput) { $launchCommand += '--disable-background-input' }
if ($LaunchReceipt) { $launchCommand += @('--launch-receipt', $LaunchReceipt) }
& $python @launchCommand
if ($LASTEXITCODE -ne 0) { throw 'Background game launcher failed.' }
