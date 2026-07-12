@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

set "PROJECT_FILE=%~dp0hc32f334_ac.uvprojx"
set "LOG_FILE=%~dp0compile.log"
set "BUILD_OPTION=-b"
set "BUILD_NAME=Build"

if "%~1"=="" goto arguments_ready
if /I "%~1"=="-b" goto arguments_ready
if /I "%~1"=="build" goto arguments_ready
if /I "%~1"=="-r" (
    set "BUILD_OPTION=-r"
    set "BUILD_NAME=Rebuild"
    goto arguments_ready
)
if /I "%~1"=="rebuild" (
    set "BUILD_OPTION=-r"
    set "BUILD_NAME=Rebuild"
    goto arguments_ready
)
if /I "%~1"=="-h" goto show_usage
if /I "%~1"=="/?" goto show_usage

echo [ERROR] Unsupported argument: %~1
call :show_usage
exit /b 1

:arguments_ready
if not exist "%PROJECT_FILE%" (
    echo [ERROR] Keil project was not found: %PROJECT_FILE%
    exit /b 1
)

call :find_uv4
if errorlevel 1 exit /b 1

if exist "%LOG_FILE%" del /f /q "%LOG_FILE%"

echo %BUILD_NAME% project: %PROJECT_FILE%
echo UV4: %UV4_EXE%
echo Log: %LOG_FILE%
echo.

set "UV4_ARGUMENTS=%BUILD_OPTION% "%PROJECT_FILE%" -j0 -o "%LOG_FILE%""
powershell.exe -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -Command "$process = Start-Process -FilePath $env:UV4_EXE -ArgumentList $env:UV4_ARGUMENTS -WindowStyle Hidden -Wait -PassThru; exit $process.ExitCode"
set "UV4_RESULT=%ERRORLEVEL%"

if exist "%LOG_FILE%" type "%LOG_FILE%"

if not "%UV4_RESULT%"=="0" (
    echo [ERROR] Keil returned exit code %UV4_RESULT%.
    exit /b %UV4_RESULT%
)

if not exist "%LOG_FILE%" (
    echo [ERROR] Keil did not create the build log.
    exit /b 1
)

findstr /I /C:" - 0 Error(s)" "%LOG_FILE%" >nul
if errorlevel 1 (
    echo [ERROR] Keil build did not report 0 errors.
    exit /b 1
)

echo.
echo Keil build completed successfully.
exit /b 0

:find_uv4
if defined UV4_EXE if exist "%UV4_EXE%" exit /b 0
set "UV4_EXE="

for %%P in (
    "%LocalAppData%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles(x86)%\Keil_v5\UV4\UV4.exe"
) do (
    if exist "%%~P" (
        set "UV4_EXE=%%~fP"
        exit /b 0
    )
)

for /f "delims=" %%I in ('where UV4.exe 2^>nul') do (
    set "UV4_EXE=%%~fI"
    exit /b 0
)

echo [ERROR] UV4.exe was not found.
echo Install Keil MDK or set UV4_EXE to its full path.
exit /b 1

:show_usage
echo.
echo Usage: %~nx0 [build^|-b^|rebuild^|-r]
echo   build    Incremental Keil build. This is the default.
echo   rebuild  Rebuild all project files.
exit /b 0
