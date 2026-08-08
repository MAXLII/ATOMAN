@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "BUILD_MODE=build"
if /I "%~1"=="-r" set "BUILD_MODE=rebuild"
if /I "%~1"=="-c" set "BUILD_MODE=clean"
if /I "%~1"=="-b" set "BUILD_MODE=build"

if not "%~1"=="" if /I not "%~1"=="-r" if /I not "%~1"=="-c" if /I not "%~1"=="-b" (
    echo Error: unsupported argument "%~1"
    exit /b 1
)

if /I "%BUILD_MODE%"=="clean" (
    mingw32-make.exe clean
    if errorlevel 1 exit /b 1
    exit /b 0
)

if /I "%BUILD_MODE%"=="rebuild" (
    mingw32-make.exe clean
    if errorlevel 1 exit /b 1
)

mingw32-make.exe -j10
if errorlevel 1 (
    echo GD32E507 firmware build failed.
    exit /b 1
)

echo GD32E507 firmware build completed.
endlocal
