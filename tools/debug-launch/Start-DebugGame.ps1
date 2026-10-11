param([switch]$DirectLevel)
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$game = Join-Path $workspace 'Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded'
$exe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
if (Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue) {
    throw 'Exit the current game first; this launcher does not terminate it.'
}
$python = 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python (Join-Path $PSScriptRoot 'configure_startup.py') apply
if ($LASTEXITCODE -ne 0) { throw 'Startup config validation failed.' }
if ($DirectLevel) {
    # Experimental UE3 map URL; does not request resume of an existing checkpoint.
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList 'SP_Prison_P'
} else {
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
}
