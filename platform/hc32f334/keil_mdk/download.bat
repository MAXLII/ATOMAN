@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "DEFAULT_HEX=output_isp\hc32f334_ac_isp.hex"
set "DOWNLOAD_FAILED=1"

if /I "%~1"=="-h" goto show_usage
if /I "%~1"=="/?" goto show_usage

if not defined RUN_AFTER_DOWNLOAD (
    set "RUN_AFTER_DOWNLOAD=1"
) else if /I "%RUN_AFTER_DOWNLOAD%"=="0" (
    set "RUN_AFTER_DOWNLOAD=0"
) else if /I "%RUN_AFTER_DOWNLOAD%"=="false" (
    set "RUN_AFTER_DOWNLOAD=0"
) else (
    set "RUN_AFTER_DOWNLOAD=1"
)

if "%RUN_AFTER_DOWNLOAD%"=="1" (
    set "RUN_AFTER_DOWNLOAD_TEXT=Enabled"
) else (
    set "RUN_AFTER_DOWNLOAD_TEXT=Disabled"
)

title Keil Download Tool

echo ========================================
echo         Keil Download Tool
echo ========================================
echo.

set "PROJECT_ARG_CONSUMED="
call :select_project "%~1"
if errorlevel 1 exit /b 1
if defined PROJECT_ARG_CONSUMED shift

call :find_uv4
if errorlevel 1 goto end

call :find_jlink
if errorlevel 1 goto end

set "TEMPLATE_PROJECT=%~dp0hc32f334_ac.uvprojx"
set "TEMPLATE_OPTIONS=%~dp0hc32f334_ac.uvoptx"
set "KEIL_PROJECT=%~dp0.hc32f334_flash.uvprojx"
set "KEIL_OPTIONS=%~dp0.hc32f334_flash.uvoptx"
set "KEIL_TARGET=AC_Download"
set "LOG_FILE=%~dp0download_%PROJECT%.log"
set "RUN_LOG_FILE=%~dp0JLinkRun_%PROJECT%.log"

echo Project: %PROJECT_NAME%
echo MCU: HC32F334KATI
echo Downloader: Keil UV4 + J-Link
echo UV4: %UV4_EXE%
echo J-Link: %JLINK_EXE%
echo Run after download: %RUN_AFTER_DOWNLOAD_TEXT%
echo.

if "%~1"=="" (
    echo Use default firmware: %DEFAULT_HEX%
    set "FILE_PATH=%DEFAULT_HEX%"
    goto check_file
)

if not exist "%~1" (
    echo ERROR: file "%~1" does not exist.
    goto end
)

set "FILE_PATH=%~f1"
goto process_file

:check_file
if not exist "%FILE_PATH%" (
    echo ERROR: fixed firmware file does not exist.
    echo Path: %FILE_PATH%
    goto end
)

:process_file
for %%F in ("%FILE_PATH%") do (
    set "FILE_PATH=%%~fF"
    set "FILE_NAME=%%~nxF"
    set "FILE_EXT=%%~xF"
)

if /I not "%FILE_EXT%"==".hex" (
    echo ERROR: Keil command-line download currently supports only .hex files.
    echo File: %FILE_NAME%
    goto end
)

if not exist "%TEMPLATE_PROJECT%" (
    echo ERROR: Keil project template does not exist.
    echo Path: %TEMPLATE_PROJECT%
    goto end
)

if not exist "%TEMPLATE_OPTIONS%" (
    echo ERROR: Keil project options do not exist.
    echo Path: %TEMPLATE_OPTIONS%
    goto end
)

call :create_flash_project
if errorlevel 1 goto end

echo Start download: %FILE_NAME%
echo Source file: %FILE_PATH%
echo.

if exist "%LOG_FILE%" del "%LOG_FILE%"
set "UV4_ARGUMENTS=-f "%KEIL_PROJECT%" -t "%KEIL_TARGET%" -j0 -o "%LOG_FILE%""
powershell.exe -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -Command "$process = Start-Process -FilePath $env:UV4_EXE -ArgumentList $env:UV4_ARGUMENTS -WindowStyle Hidden -Wait -PassThru; exit $process.ExitCode"

set "DOWNLOAD_FAILED=0"
set "UV4_RC=%ERRORLEVEL%"

if not "%UV4_RC%"=="0" (
    echo ERROR: Keil download process returned exit code %UV4_RC%.
    set "DOWNLOAD_FAILED=1"
)

if not exist "%LOG_FILE%" set "DOWNLOAD_FAILED=1"

if "%DOWNLOAD_FAILED%"=="0" (
    findstr /I /C:"*** error" /C:"Error:" /C:"Could not load file" /C:"Flash Download failed" /C:"No ULINK device found" /C:"No J-LINK device found" "%LOG_FILE%" >nul 2>nul
    if not errorlevel 1 set "DOWNLOAD_FAILED=1"
)

if "%DOWNLOAD_FAILED%"=="0" (
    findstr /I /C:"Verify OK" "%LOG_FILE%" >nul 2>nul
    if errorlevel 1 set "DOWNLOAD_FAILED=1"
)

if "%DOWNLOAD_FAILED%"=="1" (
    echo.
    echo Download failed.
    echo Check log: %LOG_FILE%
) else (
    if "%RUN_AFTER_DOWNLOAD%"=="1" (
        call :run_target
        if errorlevel 1 set "DOWNLOAD_FAILED=1"
    )
)

if "%DOWNLOAD_FAILED%"=="1" (
    echo.
    echo Download failed.
    if exist "%RUN_LOG_FILE%" (
        echo Check logs: %LOG_FILE% and %RUN_LOG_FILE%
    ) else (
        echo Check log: %LOG_FILE%
    )
) else (
    echo.
    echo Download completed successfully.
    echo Log file: %LOG_FILE%
    if "%RUN_AFTER_DOWNLOAD%"=="1" (
        echo MCU is now running.
    ) else (
        echo Additional J-Link go command was skipped.
    )
)

:end
call :cleanup_flash_project
exit /b %DOWNLOAD_FAILED%

:show_usage
echo.
echo Usage:
echo   %~nx0                  Download the default firmware.
echo   %~nx0 AC               Download the default AC firmware.
echo   %~nx0 ^<firmware.hex^>  Download a specified HEX file.
echo   %~nx0 AC ^<firmware.hex^>
echo Default firmware: %DEFAULT_HEX%
echo Environment:
echo   RUN_AFTER_DOWNLOAD=0  Skip the extra J-Link go command.
exit /b 0

:create_flash_project
powershell.exe -NoLogo -NoProfile -NonInteractive -Command "try { $utf8 = [System.Text.UTF8Encoding]::new($false); $outputDirectory = [System.Security.SecurityElement]::Escape((Split-Path -Parent $env:FILE_PATH) + '\'); $outputName = [System.Security.SecurityElement]::Escape([System.IO.Path]::GetFileName($env:FILE_PATH)); $project = [System.IO.File]::ReadAllText($env:TEMPLATE_PROJECT, $utf8); $project = $project.Replace('<TargetName>ISP</TargetName>', '<TargetName>' + $env:KEIL_TARGET + '</TargetName>'); $project = $project.Replace('<OutputDirectory>.\output_isp\</OutputDirectory>', '<OutputDirectory>' + $outputDirectory + '</OutputDirectory>'); $project = $project.Replace('<OutputName>hc32f334_ac_isp</OutputName>', '<OutputName>' + $outputName + '</OutputName>'); $project = $project.Replace('<InvalidFlash>1</InvalidFlash>', '<InvalidFlash>0</InvalidFlash>'); $project = $project.Replace('<Flash4>.\config\debug_init.ini</Flash4>', '<Flash4></Flash4>'); [System.IO.File]::WriteAllText($env:KEIL_PROJECT, $project, $utf8); $options = [System.IO.File]::ReadAllText($env:TEMPLATE_OPTIONS, $utf8); $options = $options.Replace('<TargetName>ISP</TargetName>', '<TargetName>' + $env:KEIL_TARGET + '</TargetName>'); [System.IO.File]::WriteAllText($env:KEIL_OPTIONS, $options, $utf8); exit 0 } catch { Write-Error $_; exit 1 }"
exit /b %ERRORLEVEL%

:cleanup_flash_project
if /I "%KEEP_FLASH_PROJECT%"=="1" exit /b 0
if defined KEIL_PROJECT if exist "%KEIL_PROJECT%" del /f /q "%KEIL_PROJECT%"
if defined KEIL_OPTIONS if exist "%KEIL_OPTIONS%" del /f /q "%KEIL_OPTIONS%"
exit /b 0

:find_uv4
set "UV4_EXE="

for %%P in (
    "%LocalAppData%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles(x86)%\Keil_v5\UV4\UV4.exe"
) do (
    if exist %%~P (
        set "UV4_EXE=%%~fP"
        exit /b 0
    )
)

for %%P in (UV4.exe) do (
    for /f "delims=" %%I in ('where %%P 2^>nul') do (
        set "UV4_EXE=%%~fI"
        exit /b 0
    )
)

echo ERROR: UV4.exe not found.
echo Expected under LocalAppData or Keil_v5 install directory.
exit /b 1

:find_jlink
set "JLINK_EXE="

for %%P in (
    "%ProgramFiles%\SEGGER\JLink\JLink.exe"
    "%ProgramFiles(x86)%\SEGGER\JLink\JLink.exe"
    "%ProgramFiles%\SEGGER\JLink_V936\JLink.exe"
    "%ProgramFiles(x86)%\SEGGER\JLink_V936\JLink.exe"
) do (
    if exist %%~P (
        set "JLINK_EXE=%%~fP"
        exit /b 0
    )
)

for /d %%D in ("%ProgramFiles%\SEGGER\JLink*") do (
    if exist "%%~fD\JLink.exe" (
        set "JLINK_EXE=%%~fD\JLink.exe"
        exit /b 0
    )
)

if defined ProgramFiles(x86) (
    for /d %%D in ("%ProgramFiles(x86)%\SEGGER\JLink*") do (
        if exist "%%~fD\JLink.exe" (
            set "JLINK_EXE=%%~fD\JLink.exe"
            exit /b 0
        )
    )
)

echo ERROR: JLink.exe not found.
echo Expected under Program Files\\SEGGER.
exit /b 1

:run_target
set "RUN_SCRIPT=%TEMP%\jlink_run_%PROJECT%.tmp"
if exist "%RUN_LOG_FILE%" del "%RUN_LOG_FILE%"
(
echo device Cortex-M4
echo si SWD
echo speed 4000
echo r
echo g
echo exit
) > "%RUN_SCRIPT%"

set "JLINK_ARGUMENTS=-CommanderScript "%RUN_SCRIPT%" -Log "%RUN_LOG_FILE%""
powershell.exe -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -Command "$process = Start-Process -FilePath $env:JLINK_EXE -ArgumentList $env:JLINK_ARGUMENTS -WindowStyle Hidden -Wait -PassThru; exit $process.ExitCode"
set "RUN_FAILED=0"

if errorlevel 1 set "RUN_FAILED=1"

findstr /I /C:"Cannot connect to target." /C:"Could not find emulator with USB serial number" /C:"Connecting to target via SWD failed" "%RUN_LOG_FILE%" >nul 2>nul
if not errorlevel 1 set "RUN_FAILED=1"

if exist "%RUN_SCRIPT%" del "%RUN_SCRIPT%"
exit /b %RUN_FAILED%

:select_project
if "%~1"=="" goto prompt_project
if /I "%~1"=="AC" (
    set "PROJECT_ARG_CONSUMED=1"
    call :set_project_ac
    exit /b 0
)
call :set_project_ac
exit /b 0

:prompt_project
set /p "PROJECT_CHOICE=Select project (1=AC): "
if "%PROJECT_CHOICE%"=="1" (
    call :set_project_ac
    exit /b 0
)
if /I "%PROJECT_CHOICE%"=="AC" (
    call :set_project_ac
    exit /b 0
)
echo ERROR: invalid project
exit /b 1

:set_project_ac
set "PROJECT=ac"
set "PROJECT_NAME=AC"
exit /b 0
