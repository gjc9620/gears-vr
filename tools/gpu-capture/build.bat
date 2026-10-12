@echo off
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
lib /nologo /def:renderdoc.def /machine:x64 /out:build\renderdoc.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 /DRENDERDOC_PLATFORM_WIN32 /I ..\third-party\renderdoc-download\replay-sdk target_control.cpp /Fe:build\TargetControl.exe /Fo:build\target_control.obj /link build\renderdoc.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 d3d12_smoke.cpp /Fe:build\D3D12Smoke.exe /Fo:build\d3d12_smoke.obj /link d3d12.lib dxgi.lib user32.lib
exit /b %errorlevel%
