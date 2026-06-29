@echo off
chcp 65001 >nul
setlocal

set "PORT=%~1"
set "BAUD=%~2"
set "TIMEOUT_SEC=%~3"
if "%PORT%"=="" set "PORT=COM8"
if "%BAUD%"=="" set "BAUD=921600"
if "%TIMEOUT_SEC%"=="" set "TIMEOUT_SEC=120"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sfra_sweep_debug.ps1" -Port "%PORT%" -Baud %BAUD% -TimeoutSec %TIMEOUT_SEC%
exit /b %ERRORLEVEL%
