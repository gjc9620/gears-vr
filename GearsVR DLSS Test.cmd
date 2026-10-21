@echo off
setlocal
cd /d "%~dp0"
set "GEARSVR_DLSS_PYTHON=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if exist "%GEARSVR_DLSS_PYTHON%" (
    "%GEARSVR_DLSS_PYTHON%" "%~dp0tools\vr-launcher\dlss_manual.py"
) else (
    py -3 "%~dp0tools\vr-launcher\dlss_manual.py"
)
if errorlevel 1 pause
endlocal
