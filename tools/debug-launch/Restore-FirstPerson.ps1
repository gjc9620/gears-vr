$ErrorActionPreference = 'Stop'
$pythonExe = 'C:/Users/Player/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $pythonExe (Join-Path $PSScriptRoot 'first_person_mod.py') restore
if ($LASTEXITCODE -ne 0) { throw 'First-person package restore failed; original backups remain in analysis/backups/first-person-1.0.' }
