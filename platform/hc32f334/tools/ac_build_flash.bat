@echo off
chcp 65001 >nul
setlocal

call "%~dp0ac_build.bat"
if errorlevel 1 exit /b 1

call "%~dp0ac_flash.bat"
exit /b %ERRORLEVEL%
