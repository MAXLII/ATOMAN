@echo off
setlocal
set SCRIPT_DIR=%~dp0
set PORT=%~1
set BAUD=%~2
if "%PORT%"=="" set PORT=COM8
if "%BAUD%"=="" set BAUD=921600
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%protocol_test.ps1" -Port "%PORT%" -Baud %BAUD%
exit /b %ERRORLEVEL%
