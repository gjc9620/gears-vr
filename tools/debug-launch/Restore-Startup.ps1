$ErrorActionPreference = 'Stop'
$python = 'C:/Users/Player/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python (Join-Path $PSScriptRoot 'configure_startup.py') restore
if ($LASTEXITCODE -ne 0) { throw 'Restore failed; see the error above.' }
