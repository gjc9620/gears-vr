param([switch]$DirectLevel, [switch]$PauseOnUnfocus, [switch]$Foreground,
      [switch]$DisableBackgroundInput = $true, [string]$GamePath, [switch]$PythonCheckOnly, [string]$LaunchReceipt,
      [switch]$ExperimentalPreserveUpscaler)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$OutputEncoding = [Console]::OutputEncoding
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
function Find-GearsVrPython([string]$Root) {
    $preferred = [Environment]::GetEnvironmentVariable('GEARSVR_PYTHON')
    $explicitPython = -not [string]::IsNullOrWhiteSpace($preferred)
    $candidates = if ($explicitPython) {
        @([Environment]::ExpandEnvironmentVariables($preferred.Trim().Trim('"')))
    } else {
        @((Join-Path $Root 'runtime/python/python.exe'),
          (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'))
    }
    foreach ($candidate in $candidates) {
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
            $output = $probe.StandardOutput.ReadToEndAsync()
            $errors = $probe.StandardError.ReadToEndAsync()
            if (-not $probe.WaitForExit(5000)) {
                $probe.Kill() # Only this disposable interpreter locator probe.
                continue
            }
            $lines = $output.Result.Trim() -split '[\r\n]+'
            if ($probe.ExitCode -eq 0 -and $lines.Count -eq 2 -and $lines[1] -eq '3' -and
                (Test-Path -LiteralPath $lines[0] -PathType Leaf)) {
                return [IO.Path]::GetFullPath($lines[0])
            }
        } catch {
            # Explicit selection never falls through to a different interpreter.
        } finally { $probe.Dispose() }
    }
    if ($explicitPython) { throw 'GEARSVR_PYTHON is not a usable Python 3 executable; no game startup actions were taken.' }
    throw 'No usable Python 3 found in runtime/python or the development cache; set GEARSVR_PYTHON explicitly.'
}
$python = Find-GearsVrPython $workspace
$env:GEARSVR_PYTHON = $python
if ($PythonCheckOnly) {
    @{python=$python; check_only=$true} | ConvertTo-Json -Compress
    exit 0
}
$game = Join-Path $workspace 'game-installation/Gears of War Reloaded'
$exe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
if ($GamePath) { $exe = $GamePath }
$validated = & $python (Join-Path $PSScriptRoot 'game_install.py') --game-path $exe
if ($LASTEXITCODE -ne 0) { throw 'Unsupported game installation; no startup changes made.' }
$exe = [string]$validated
$defaultExe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
# A windowless helper can remain after the main game exits. Do not count it as
# a playable instance; still refuse a second visible game, including a hung one.
$isolatedInstance = -not [string]::IsNullOrEmpty($env:GEARSVR_INSTANCE_ROOT)
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
& $python (Join-Path $PSScriptRoot 'graphics_policy.py') $graphicsAction
if ($LASTEXITCODE -ne 0) { throw 'Upscaler startup policy failed.' }
# Custom installations are read-only: validation above accepts original or known
# patched config; never apply repository backups to another installation.
if ($isDefault -and -not $isolatedInstance) {
    & $python (Join-Path $PSScriptRoot 'configure_startup.py') apply
    if ($LASTEXITCODE -ne 0) { throw 'Startup config validation failed.' }
}
$launchArguments = @()
if (-not $PauseOnUnfocus) { $launchArguments += '-NoPauseUnfocus' }
if ($DirectLevel) {
    # Experimental UE3 map URL; does not request resume of an existing checkpoint.
    $launchArguments = @('SP_Prison_P') + $launchArguments
}
$launchCommand = @((Join-Path $PSScriptRoot 'background_launch.py'), '--game-path', $exe)
foreach ($argument in $launchArguments) { $launchCommand += "--game-arg=$argument" }
if ($Foreground) { $launchCommand += '--foreground' }
if ($DisableBackgroundInput) { $launchCommand += '--disable-background-input' }
if ($LaunchReceipt) { $launchCommand += @('--launch-receipt', $LaunchReceipt) }
& $python @launchCommand
if ($LASTEXITCODE -ne 0) { throw 'Background game launcher failed.' }
