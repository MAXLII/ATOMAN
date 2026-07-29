@echo off
chcp 65001 >nul
setlocal

set "PORT=%~1"
set "BAUD=%~2"
if "%PORT%"=="" set "PORT=COM8"
if "%BAUD%"=="" set "BAUD=921600"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sfra_sweep_debug.ps1" -Port "%PORT%" -Baud %BAUD% -NoReset -NoStart -InfoOnly
exit /b %ERRORLEVEL%
