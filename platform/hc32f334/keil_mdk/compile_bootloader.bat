@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "PROJECT_FILE=%~dp0hc32f334_bootloader.uvprojx"
set "LOG_FILE=%~dp0compile_bootloader.log"
set "BUILD_OPTION=-b"
if /I "%~1"=="rebuild" set "BUILD_OPTION=-r"
if /I "%~1"=="-r" set "BUILD_OPTION=-r"

if defined UV4_EXE if exist "%UV4_EXE%" goto build
set "UV4_EXE=%LocalAppData%\Keil_v5\UV4\UV4.exe"
if exist "%UV4_EXE%" goto build
set "UV4_EXE=%ProgramFiles%\Keil_v5\UV4\UV4.exe"
if exist "%UV4_EXE%" goto build
echo [ERROR] UV4.exe was not found.
exit /b 1

:build
if exist "%LOG_FILE%" del /f /q "%LOG_FILE%"
set "UV4_ARGUMENTS=%BUILD_OPTION% "%PROJECT_FILE%" -j0 -o "%LOG_FILE%""
powershell.exe -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -Command "$process = Start-Process -FilePath $env:UV4_EXE -ArgumentList $env:UV4_ARGUMENTS -WindowStyle Hidden -Wait -PassThru; exit $process.ExitCode"
set "UV4_RESULT=%ERRORLEVEL%"
if exist "%LOG_FILE%" type "%LOG_FILE%"
if not "%UV4_RESULT%"=="0" exit /b %UV4_RESULT%
findstr /I /C:" - 0 Error(s)" "%LOG_FILE%" >nul
if errorlevel 1 exit /b 1
exit /b 0
