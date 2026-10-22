@echo off
if "%~1"=="" (
    echo Usage: download.bat COM7
    exit /b 2
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0gcc\compile.ps1" -Port "%~1" -Flash
exit /b %errorlevel%
