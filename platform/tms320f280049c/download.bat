@echo off
setlocal
set "LOADTI=C:\ti\ccs2100\ccs\ccs_base\scripting\examples\loadti\loadti.bat"
set "CONFIG=platform\tms320f280049c\targetConfigs\TMS320F280049C_LaunchPad.ccxml"
set "PROGRAM=platform\tms320f280049c\build\section_task_demo.out"
for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
pushd "%REPO_ROOT%" || exit /b 1
call "%LOADTI%" -c "%CONFIG%" -r -a "%PROGRAM%"
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
