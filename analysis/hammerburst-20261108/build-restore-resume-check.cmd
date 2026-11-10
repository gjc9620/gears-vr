@echo off
call "E:\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "C:\Users\11\.codex\worktrees\6dof-single\gearsVr\tools\playground\weapon_hammerburst_restore_resume_test.cpp" /Fo"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\restore-resume-test.obj" /Fe"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\restore-resume-test.exe"
if errorlevel 1 exit /b %errorlevel%
"C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108\restore-resume-test.exe"
exit /b %errorlevel%
