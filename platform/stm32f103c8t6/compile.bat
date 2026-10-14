@echo off
setlocal
cd /d "%~dp0gcc"
set "MODE=srtos_m"
set "ACTION=build"
if /I "%~1"=="baremetal" set "MODE=baremetal"
if /I "%~1"=="srtos_m" set "MODE=srtos_m"
if not "%~1"=="" if /I not "%~1"=="baremetal" if /I not "%~1"=="srtos_m" exit /b 2
if /I "%~2"=="-r" set "ACTION=rebuild"
if /I "%~2"=="-c" set "ACTION=clean"
if not "%~2"=="" if /I not "%~2"=="-b" if /I not "%~2"=="-r" if /I not "%~2"=="-c" exit /b 2
if /I "%ACTION%"=="clean" (
    mingw32-make SECTION_RUNTIME=%MODE% clean
    if errorlevel 1 exit /b 1
    exit /b 0
)
if /I "%ACTION%"=="rebuild" (
    mingw32-make SECTION_RUNTIME=%MODE% clean
    if errorlevel 1 exit /b 1
)
mingw32-make -j8 SECTION_RUNTIME=%MODE% all
exit /b %errorlevel%
