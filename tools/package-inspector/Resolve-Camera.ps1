param(
    [int]$GameProcessId = 0,
    [string]$OutputPath = 'J:\gearsVr\analysis\runtime\camera-resolved.json'
)
$ErrorActionPreference = 'Stop'
if ($GameProcessId -eq 0) {
    $gameProcesses = @(Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue)
    if ($gameProcesses.Count -ne 1) {
        throw "Expected one GOWDE-Steam process, found $($gameProcesses.Count). Specify -GameProcessId if needed."
    }
    $GameProcessId = $gameProcesses[0].Id
}
$pythonExecutable = 'C:\Users\Player\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExecutable (Join-Path $PSScriptRoot 'resolve_camera.py') --pid $GameProcessId --output $OutputPath
if ($LASTEXITCODE -ne 0) { throw 'Camera resolution failed; no candidate accepted. See JSON error report.' }
