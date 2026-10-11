@echo off
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
if errorlevel 1 exit /b 1
cd /d "%~dp0"
set "capture_build_dir=build"
if not "%~1"=="" set "capture_build_dir=%~1"
set "capture_output_name=GearsCapture"
if not "%~2"=="" set "capture_output_name=%~2"
cmake -S . -B "%capture_build_dir%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCAPTURE_OUTPUT_NAME=%capture_output_name%
if errorlevel 1 exit /b 1
cmake --build "%capture_build_dir%"
exit /b %errorlevel%
