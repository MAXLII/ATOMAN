@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0gcc\compile.ps1" %*
exit /b %errorlevel%
