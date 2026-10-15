param(
    [ValidateSet('start','stop','status','recenter')][string]$Action = 'start',
    [string]$Python = "$env:USERPROFILE/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$launcherPath = Join-Path $PSScriptRoot 'launcher.py'
if ($Action -ne 'start') {
    & $Python $launcherPath $Action
    exit $LASTEXITCODE
}
$simulatorFolder = Join-Path $repositoryRoot 'analysis/runtime/simulator-quiet2'
$runtimeSource = Join-Path $simulatorFolder 'openxr_simulator.json'
$runtimeConfig = Get-Content -LiteralPath $runtimeSource -Raw | ConvertFrom-Json
$runtimeLibrary = [IO.Path]::GetFullPath((Join-Path $simulatorFolder $runtimeConfig.runtime.library_path))
if (-not (Test-Path -LiteralPath $runtimeLibrary -PathType Leaf)) { throw 'Simulator runtime library missing' }
$runtimeConfig.runtime.library_path = $runtimeLibrary
$runtimeDirectory = Join-Path $repositoryRoot 'analysis/runtime'
[IO.Directory]::CreateDirectory($runtimeDirectory) | Out-Null
$runtimeOutput = Join-Path $runtimeDirectory 'launcher-simulator-absolute.json'
[IO.File]::WriteAllText($runtimeOutput, ($runtimeConfig | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
& $Python $launcherPath start --manifest (Join-Path $PSScriptRoot 'manifest.json') --runtime $runtimeOutput
exit $LASTEXITCODE
