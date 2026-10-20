@echo off
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build
exit /b %errorlevel%
