@echo off
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
if errorlevel 1 exit /b 1
cd /d "%~dp0"
set "gpu_build_dir=build"
if not "%~1"=="" set "gpu_build_dir=%~1"
if not exist "%gpu_build_dir%" mkdir "%gpu_build_dir%"
lib /nologo /def:renderdoc.def /machine:x64 /out:"%gpu_build_dir%\renderdoc.lib"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 /DRENDERDOC_PLATFORM_WIN32 /I ..\third-party\renderdoc-download\replay-sdk target_control.cpp /Fe:"%gpu_build_dir%\TargetControl.exe" /Fo:"%gpu_build_dir%\target_control.obj" /link "%gpu_build_dir%\renderdoc.lib"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 d3d12_smoke.cpp /Fe:"%gpu_build_dir%\D3D12Smoke.exe" /Fo:"%gpu_build_dir%\d3d12_smoke.obj" /link d3d12.lib dxgi.lib user32.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 /LD bridge_probe.cpp /Fe:"%gpu_build_dir%\BridgeProbe.dll" /Fo:"%gpu_build_dir%\bridge_probe.obj" /link d3d12.lib dxgi.lib
exit /b %errorlevel%
