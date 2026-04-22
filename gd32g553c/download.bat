@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "MCU=GD32G553RCT6"
set "JLINK_DEVICE=GD32G553RCT6"
set "IF=SWD"
set "SPEED=4000"
set "BIN_ADDR=0x08000000"
set "DEFAULT_FIRMWARE=build\demo.bin"
set "LOG_FILE=JLink_demo.log"

if not defined RUN_AFTER_DOWNLOAD (
    set "RUN_AFTER_DOWNLOAD=1"
) else if /I "%RUN_AFTER_DOWNLOAD%"=="0" (
    set "RUN_AFTER_DOWNLOAD=0"
) else if /I "%RUN_AFTER_DOWNLOAD%"=="false" (
    set "RUN_AFTER_DOWNLOAD=0"
) else (
    set "RUN_AFTER_DOWNLOAD=1"
)

if "%RUN_AFTER_DOWNLOAD%"=="1" (
    set "RUN_AFTER_DOWNLOAD_TEXT=Enabled"
) else (
    set "RUN_AFTER_DOWNLOAD_TEXT=Disabled"
)

title J-Link Download Tool

echo ========================================
echo         J-Link Download Tool
echo ========================================
echo Device: %JLINK_DEVICE%
echo Interface: %IF%
echo Speed: %SPEED% kHz
echo ========================================
echo.

call :find_jlink
if errorlevel 1 goto end

echo J-Link: %JLINK_EXE%
echo Run after download: %RUN_AFTER_DOWNLOAD_TEXT%
echo.

if "%~1"=="" (
    echo [INFO] No firmware argument provided, use default path: %DEFAULT_FIRMWARE%
    set "FILE_PATH=%DEFAULT_FIRMWARE%"
    goto check_default_file
)

if /I "%~1"=="-h" goto show_usage
if /I "%~1"=="/?" goto show_usage

if not exist "%~1" (
    echo [ERROR] File "%~1" does not exist.
    goto end
)

set "FILE_PATH=%~f1"
set "JLINK_FILE_PATH=%FILE_PATH:\=/%"
set "FILE_NAME=%~nx1"
set "FILE_EXT=%~x1"
goto process_file

:check_default_file
if not exist "%DEFAULT_FIRMWARE%" (
    echo [ERROR] Default firmware file does not exist.
    echo Check path: %DEFAULT_FIRMWARE%
    call :show_usage
    goto end
)

for %%F in ("%DEFAULT_FIRMWARE%") do (
    set "FILE_PATH=%%~fF"
    set "JLINK_FILE_PATH=%%~fF"
    set "FILE_NAME=%%~nxF"
    set "FILE_EXT=%%~xF"
)
set "JLINK_FILE_PATH=!JLINK_FILE_PATH:\=/!"

:process_file
call :get_load_command
if errorlevel 1 goto end

set "JLINK_SCRIPT=%TEMP%\jlink_script_demo.tmp"
(
echo device %JLINK_DEVICE%
echo si %IF%
echo speed %SPEED%
echo halt
echo r
echo !LOAD_CMD!
echo r
if "%RUN_AFTER_DOWNLOAD%"=="1" echo g
echo exit
) > "%JLINK_SCRIPT%"

echo Start download: !FILE_NAME!
echo File path: %FILE_PATH%
echo.

"%JLINK_EXE%" -CommanderScript "%JLINK_SCRIPT%" -Log "%LOG_FILE%"

if errorlevel 1 (
    echo.
    echo Download failed.
    echo Check log: %LOG_FILE%
) else (
    echo.
    echo Download completed successfully.
    if "%RUN_AFTER_DOWNLOAD%"=="1" (
        echo MCU is now running.
    ) else (
        echo MCU remains halted.
    )
    echo Log file: %LOG_FILE%
)

if exist "%JLINK_SCRIPT%" del "%JLINK_SCRIPT%"

:end
exit /b

:show_usage
echo.
echo Usage:
echo 1. Run directly to download the default firmware: %DEFAULT_FIRMWARE%
echo 2. Or use: %~nx0 ^<firmware_file_path^>
echo.
echo Supported formats: .hex, .bin, .s19
echo Environment:
echo   RUN_AFTER_DOWNLOAD=0  keep MCU halted after download
echo   RUN_AFTER_DOWNLOAD=1  run MCU after download
exit /b 0

:find_jlink
set "JLINK_EXE="

if exist "%ProgramFiles%\SEGGER\JLink\JLink.exe" (
    set "JLINK_EXE=%ProgramFiles%\SEGGER\JLink\JLink.exe"
    exit /b 0
)

if defined ProgramFiles(x86) (
    if exist "%ProgramFiles(x86)%\SEGGER\JLink\JLink.exe" (
        set "JLINK_EXE=%ProgramFiles(x86)%\SEGGER\JLink\JLink.exe"
        exit /b 0
    )
)

for /d %%D in ("%ProgramFiles%\SEGGER\JLink*") do (
    if exist "%%~fD\JLink.exe" (
        set "JLINK_EXE=%%~fD\JLink.exe"
        exit /b 0
    )
)

if defined ProgramFiles(x86) (
    for /d %%D in ("%ProgramFiles(x86)%\SEGGER\JLink*") do (
        if exist "%%~fD\JLink.exe" (
            set "JLINK_EXE=%%~fD\JLink.exe"
            exit /b 0
        )
    )
)

echo [ERROR] JLink.exe not found.
echo Expected under "%ProgramFiles%\SEGGER" or "%ProgramFiles(x86)%\SEGGER".
exit /b 1

:get_load_command
if /I "!FILE_EXT!"==".hex" (
    set "LOAD_CMD=loadfile !JLINK_FILE_PATH!"
    exit /b 0
)

if /I "!FILE_EXT!"==".bin" (
    echo [INFO] .bin file detected, use address: %BIN_ADDR%
    set "LOAD_CMD=loadfile !JLINK_FILE_PATH! %BIN_ADDR%"
    exit /b 0
)

if /I "!FILE_EXT!"==".s19" (
    set "LOAD_CMD=loadfile !JLINK_FILE_PATH!"
    exit /b 0
)

echo [ERROR] Unsupported file extension: !FILE_EXT!
echo Supported formats: .hex, .bin, .s19
exit /b 1
