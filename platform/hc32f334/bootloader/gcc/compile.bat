@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "MAKE_EXE=C:\MinGW\bin\mingw32-make.exe"
if not exist "%MAKE_EXE%" set "MAKE_EXE=mingw32-make.exe"

if /I "%~1"=="clean" (
    "%MAKE_EXE%" clean
    exit /b %ERRORLEVEL%
)
if /I "%~1"=="rebuild" (
    "%MAKE_EXE%" clean
    if errorlevel 1 exit /b 1
)

"%MAKE_EXE%" -j10
exit /b %ERRORLEVEL%
