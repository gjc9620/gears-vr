$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$runtime = 'C:/Users/Player/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$state = Join-Path $workspace 'analysis/game-io'
New-Item -ItemType Directory -Force -Path $state | Out-Null
$arguments = @('"' + (Join-Path $PSScriptRoot 'game_io.py') + '"', 'serve')
$service = Start-Process -FilePath $runtime -ArgumentList $arguments -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput (Join-Path $state 'server-stdout.log') `
    -RedirectStandardError (Join-Path $state 'server-stderr.log')
Write-Output "Game I/O service PID: $($service.Id)"
