@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "BUILD_MODE=build"

if "%~1"=="" goto arguments_ready
if /I "%~1"=="-b" goto arguments_ready
if /I "%~1"=="build" goto arguments_ready
if /I "%~1"=="-r" (
    set "BUILD_MODE=rebuild"
    goto arguments_ready
)
if /I "%~1"=="rebuild" (
    set "BUILD_MODE=rebuild"
    goto arguments_ready
)
if /I "%~1"=="-c" (
    set "BUILD_MODE=clean"
    goto arguments_ready
)
if /I "%~1"=="clean" (
    set "BUILD_MODE=clean"
    goto arguments_ready
)
if /I "%~1"=="-h" goto show_usage
if /I "%~1"=="/?" goto show_usage

echo [ERROR] Unsupported argument: %~1
call :show_usage
exit /b 1

:arguments_ready
call :find_make
if errorlevel 1 exit /b 1

if not defined GCC_PATH (
    where arm-none-eabi-gcc.exe >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] arm-none-eabi-gcc.exe was not found in PATH.
        echo Set GCC_PATH to the Arm GNU Toolchain bin directory or add it to PATH.
        exit /b 1
    )
)

echo Build mode: %BUILD_MODE%
echo Make tool: %MAKE_EXE%

if /I "%BUILD_MODE%"=="clean" (
    "%MAKE_EXE%" -s clean
    exit /b %ERRORLEVEL%
)

if /I "%BUILD_MODE%"=="rebuild" (
    "%MAKE_EXE%" -s clean
    if errorlevel 1 exit /b 1
)

"%MAKE_EXE%" -s -j10
exit /b %ERRORLEVEL%

:find_make
set "MAKE_EXE="

for %%M in (mingw32-make.exe make.exe) do (
    for /f "delims=" %%I in ('where %%M 2^>nul') do (
        set "MAKE_EXE=%%~fI"
        exit /b 0
    )
)

echo [ERROR] mingw32-make.exe or make.exe was not found in PATH.
exit /b 1

:show_usage
echo.
echo Usage: %~nx0 [build^|-b^|rebuild^|-r^|clean^|-c]
echo   build    Incremental build. This is the default.
echo   rebuild  Clean and build all files.
echo   clean    Remove GCC build outputs.
exit /b 0
