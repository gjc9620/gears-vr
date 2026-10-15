param([switch]$DirectLevel, [switch]$PauseOnUnfocus, [switch]$Foreground,
      [switch]$DisableBackgroundInput, [string]$GamePath)
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$game = Join-Path $workspace 'Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded'
$exe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
if ($GamePath) { $exe = $GamePath }
$python = 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$validated = & $python (Join-Path $PSScriptRoot 'game_install.py') --game-path $exe
if ($LASTEXITCODE -ne 0) { throw 'Unsupported game installation; no startup changes made.' }
$exe = [string]$validated
$defaultExe = Join-Path $game 'Binaries_x64/GOWDE-Steam.exe'
# A windowless helper can remain after the main game exits. Do not count it as
# a playable instance; still refuse a second visible game, including a hung one.
if (Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue | Where-Object {$_.MainWindowHandle -ne 0}) {
    throw 'Exit the current game first; this launcher does not terminate it.'
}
# Preserve the default reversible policy. Other installs require DLSS already
# OFF and make no writes to either installation or the player's user settings.
$isDefault = [IO.Path]::GetFullPath($exe) -eq [IO.Path]::GetFullPath($defaultExe)
$graphicsAction = if ($isDefault) { 'apply' } else { 'check' }
& $python (Join-Path $PSScriptRoot 'graphics_policy.py') $graphicsAction
if ($LASTEXITCODE -ne 0) { throw 'DLSS OFF startup policy failed.' }
# Custom installations are read-only: validation above accepts original or known
# patched config; never apply repository backups to another installation.
if ($isDefault) {
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
& $python @launchCommand
if ($LASTEXITCODE -ne 0) { throw 'Background game launcher failed.' }
