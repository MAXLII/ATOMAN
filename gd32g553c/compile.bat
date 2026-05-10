@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
cd /d "%~dp0"

title Firmware Build Tool

echo ========================================
echo       Firmware Build Tool
echo ========================================
echo.

set "BUILD_MODE=build"

call :parse_args %*
if errorlevel 1 exit /b 1

echo Build mode: %BUILD_MODE%
echo.

if /I "%BUILD_MODE%"=="clean" goto clean_only

if /I "%BUILD_MODE%"=="rebuild" (
    echo Step 1: clean build directory...
    call :clean_build_dir
    if errorlevel 1 goto error
) else (
    echo Step 1: skip clean build directory...
)

echo Step 2: build fw_info tool...
if exist fw_info.exe del /f /q fw_info.exe
gcc -o fw_info.exe fw_info.c -DIS_PFC
if errorlevel 1 (
    echo Error: failed to build fw_info.exe.
    goto error
)
echo fw_info.exe built.

echo Step 3: build firmware...
mingw32-make.exe -s -j10 OUTPUT_NAME=isp_firmware
if errorlevel 1 (
    echo Error: firmware build failed.
    goto error
)
echo Firmware build completed.

echo Step 4: append firmware info...
for /f %%i in ('powershell -command "Get-Date -Format yyyyMMddHHmmss"') do set "datetime=%%i"
set "target_dir=builds\!datetime!"
if not exist "!target_dir!" mkdir "!target_dir!"
call :run_fw_info
if errorlevel 1 (
    echo Error: failed to append firmware info.
    goto error
)
echo Output: !target_dir!\demo.bin

echo.
echo Build finished successfully.
goto end

:clean_only
echo Step 1: clean build directory...
call :clean_build_dir
if errorlevel 1 goto error
echo.
echo Clean completed successfully.
goto end

:error
echo.
echo Build failed.
exit /b 1

:parse_args
if "%~1"=="" exit /b 0

if /I "%~1"=="-r" (
    set "BUILD_MODE=rebuild"
    exit /b 0
)

if /I "%~1"=="-c" (
    set "BUILD_MODE=clean"
    exit /b 0
)

if /I "%~1"=="-b" (
    set "BUILD_MODE=build"
    exit /b 0
)

echo Error: unsupported argument "%~1"
exit /b 1

:clean_build_dir
if exist "build" rmdir /s /q "build"
exit /b 0

:run_fw_info
.\fw_info.exe build\demo.bin !target_dir!\demo.bin
exit /b 0

:end
endlocal