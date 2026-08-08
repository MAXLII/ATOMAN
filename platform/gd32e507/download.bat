@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "JLINK_EXE=C:\Program Files\SEGGER\JLink_V936\JLink.exe"

if not exist "%JLINK_EXE%" (
    echo Error: J-Link executable was not found.
    exit /b 1
)

if not exist "build\gd32e507_demo.hex" (
    echo Error: firmware image was not found. Run compile.bat first.
    exit /b 1
)

"%JLINK_EXE%" -NoGui 1 -device GD32E507ZE -if SWD -speed 1000 -autoconnect 1 -ExitOnError 1 -CommandFile download.jlink
if errorlevel 1 (
    echo GD32E507 firmware download failed.
    exit /b 1
)

echo GD32E507 firmware download completed.
endlocal
