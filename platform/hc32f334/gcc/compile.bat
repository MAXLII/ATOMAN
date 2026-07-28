@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "FIRMWARE_TARGET=ISP"
set "MAKE_TARGET=BUILD_TARGET=isp"
set "BUILD_MODE=build"

if /I "%~1"=="-iap" (
    set "FIRMWARE_TARGET=IAP"
    set "MAKE_TARGET=BUILD_TARGET=iap"
    shift
)
if /I "%~1"=="-isp" shift

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
echo Firmware target: %FIRMWARE_TARGET%
echo Make tool: %MAKE_EXE%
echo.

if /I "%BUILD_MODE%"=="clean" goto clean_target
if /I "%BUILD_MODE%"=="rebuild" goto rebuild_target
goto build_target

:clean_target
"%MAKE_EXE%" -s %MAKE_TARGET% clean
set "BUILD_RESULT=%ERRORLEVEL%"
goto finish

:rebuild_target
"%MAKE_EXE%" -s %MAKE_TARGET% clean
if errorlevel 1 (
    set "BUILD_RESULT=1"
    goto finish
)

:build_target
"%MAKE_EXE%" -s %MAKE_TARGET% -j10
set "BUILD_RESULT=%ERRORLEVEL%"

:finish
exit /b %BUILD_RESULT%

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
echo Usage: %~nx0 [-isp^|-iap] [build^|-b^|rebuild^|-r^|clean^|-c]
echo   -isp     Build the ISP target. This is the default.
echo   -iap     Build the IAP target.
echo   build    Incremental build. This is the default.
echo   rebuild  Clean and build all files.
echo   clean    Remove GCC build outputs.
exit /b 0
