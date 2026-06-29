@echo off
chcp 65001 >nul
setlocal

for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "PROJECT_DIR=%ROOT%\hc32f334\ac\keil_mdk"
set "PROJECT_FILE=%PROJECT_DIR%\hc32f334_ac.uvprojx"
set "LOG_FILE=%PROJECT_DIR%\codex_build.log"

call "%~dp0keil_env.bat"
if errorlevel 1 exit /b 1

if not exist "%PROJECT_FILE%" (
    echo ERROR: project file not found.
    echo %PROJECT_FILE%
    exit /b 1
)

echo Build project: %PROJECT_FILE%
echo Log: %LOG_FILE%
"%UV4_EXE%" -b "%PROJECT_FILE%" -j0 -o "%LOG_FILE%"
set "UV4_RC=%ERRORLEVEL%"

type "%LOG_FILE%"
if not "%UV4_RC%"=="0" exit /b %UV4_RC%

findstr /I /C:" - 0 Error(s)" "%LOG_FILE%" >nul
if errorlevel 1 exit /b 1

exit /b 0
