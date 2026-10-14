@echo off
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
if errorlevel 1 exit /b 1
cd /d "%~dp0"
set "xr_build_dir=build"
if not "%~1"=="" set "xr_build_dir=%~1"
cmake -S . -B "%xr_build_dir%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build "%xr_build_dir%"
exit /b %errorlevel%
