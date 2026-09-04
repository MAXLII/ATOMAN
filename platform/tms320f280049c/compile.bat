@echo off
setlocal
set "MAKE=C:\ti\ccs2100\ccs\utils\bin\gmake.exe"
for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
pushd "%REPO_ROOT%" || exit /b 1
"%MAKE%" -f platform/tms320f280049c/Makefile all
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
