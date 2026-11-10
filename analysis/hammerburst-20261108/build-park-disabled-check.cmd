@echo off
call "E:\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /DHB_PARK_TEST_DISABLED=1 /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "C:\Users\11\.codex\worktrees\6dof-single\gearsVr\tools\playground\weapon_hammerburst_reload_park_test.cpp" /Fo"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\park-disabled-test.obj" /Fe"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\park-disabled-test.exe"
if errorlevel 1 exit /b %errorlevel%
"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\park-disabled-test.exe"
exit /b %errorlevel%
