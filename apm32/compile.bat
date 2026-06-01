@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "BUILD_MODE=build"
if /I "%~1"=="-r" set "BUILD_MODE=rebuild"
if /I "%~1"=="rebuild" set "BUILD_MODE=rebuild"
if /I "%~1"=="-c" set "BUILD_MODE=clean"
if /I "%~1"=="clean" set "BUILD_MODE=clean"

if /I "%BUILD_MODE%"=="clean" (
    mingw32-make.exe -s clean
    exit /b %errorlevel%
)

if /I "%BUILD_MODE%"=="rebuild" (
    mingw32-make.exe -s clean
    if errorlevel 1 exit /b 1
)

mingw32-make.exe -s -j10
exit /b %errorlevel%
