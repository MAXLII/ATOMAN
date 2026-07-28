@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "TARGET_CORE=Cortex-M4"
set "DEFAULT_FIRMWARE=build\isp\hc32f334_ac_isp.hex"
set "MDK_DOWNLOADER=%~dp0..\keil_mdk\download.bat"

if /I "%~1"=="-h" goto show_usage
if /I "%~1"=="/?" goto show_usage

if "%~1"=="" (
    set "FILE_PATH=%DEFAULT_FIRMWARE%"
) else (
    set "FILE_PATH=%~1"
)

if not exist "%FILE_PATH%" (
    echo [ERROR] Firmware file does not exist: %FILE_PATH%
    exit /b 1
)

for %%F in ("%FILE_PATH%") do (
    set "FILE_PATH=%%~fF"
    set "FILE_EXT=%%~xF"
)

if /I not "%FILE_EXT%"==".hex" (
    echo [ERROR] HC32F334 Flash download requires an Intel HEX file.
    echo File: %FILE_PATH%
    exit /b 1
)

if not exist "%MDK_DOWNLOADER%" (
    echo [ERROR] Keil Flash downloader was not found: %MDK_DOWNLOADER%
    exit /b 1
)

echo Target core: %TARGET_CORE%
echo Firmware: %FILE_PATH%
echo Programmer: Keil HC32F334 Flash algorithm with J-Link
echo.

call "%MDK_DOWNLOADER%" AC "%FILE_PATH%"
set "DOWNLOAD_RESULT=%ERRORLEVEL%"

if not "%DOWNLOAD_RESULT%"=="0" (
    echo.
    echo [ERROR] GCC firmware download failed.
    exit /b %DOWNLOAD_RESULT%
)

echo.
echo GCC firmware download and verification completed successfully.
exit /b 0

:show_usage
echo.
echo Usage: %~nx0 [firmware.hex]
echo Default firmware: %DEFAULT_FIRMWARE%
echo Target core: %TARGET_CORE%
echo The HC32F334 Keil Pack supplies the required on-chip Flash algorithm.
echo Environment:
echo   RUN_AFTER_DOWNLOAD=0  Skip the extra J-Link go command.
exit /b 0
