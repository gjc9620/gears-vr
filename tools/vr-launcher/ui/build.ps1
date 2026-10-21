param([string]$OutputDirectory, [switch]$NoInstall)
$ErrorActionPreference = 'Stop'
$repository = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    $compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework/v4.0.30319/csc.exe'
}
if (-not (Test-Path -LiteralPath $compiler)) { throw '.NET Framework C# compiler is unavailable; no dependencies were installed.' }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repository 'analysis/vr-launcher-ui'
}
$outputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$executable = Join-Path $outputDirectory 'GearsVrLauncher.exe'
$icon = Join-Path $PSScriptRoot 'assets/launcher.ico'
& (Join-Path $PSScriptRoot 'build-icon.ps1') -Destination $icon
$artwork = [IO.Path]::ChangeExtension($icon, '.png')
& $compiler /nologo /target:winexe /platform:x64 /optimize+ /codepage:65001 /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll "/win32icon:$icon" "/resource:$icon,GearsVrUi.LauncherIcon" "/resource:$artwork,GearsVrUi.LauncherArtwork" "/out:$executable" (Join-Path $PSScriptRoot 'GearsVrLauncher.cs')
if ($LASTEXITCODE -ne 0) { throw 'Launcher compilation failed.' }
$test = Start-Process -FilePath $executable -ArgumentList '--self-test' -PassThru -Wait -WindowStyle Hidden
if ($test.ExitCode -ne 0) { throw "Launcher self-test failed: $($test.ExitCode)" }
Get-Content -LiteralPath (Join-Path $outputDirectory 'self-test.txt')
if ($NoInstall) {
    Write-Output "Candidate launcher (not installed): $executable"
} else {
    $playerExecutable = Join-Path $repository 'GearsVR Launcher.exe'
    Copy-Item -LiteralPath $executable -Destination $playerExecutable -Force
    Write-Output "Launcher: $playerExecutable"
}
