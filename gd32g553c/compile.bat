@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
cd /d "%~dp0"

title Firmware Build Tool

echo ========================================
echo       Firmware Build Tool
echo ========================================
echo.

set "OUTPUT_NAME=isp_firmware"
set "CUSTOM_NAME="
set "BUILD_MODE=build"
set "BUILD_MODE_NAME=Incremental Build"
set "PROMPT_CUSTOM_NAME=0"

call :parse_args %*
if errorlevel 2 goto end
if errorlevel 1 exit /b 1

echo.
echo Build configuration:
echo   Type: ISP
echo   Platform: IS_DC
echo   Mode: %BUILD_MODE_NAME%
echo.

if /I "%BUILD_MODE%"=="clean" goto clean_only

if "%PROMPT_CUSTOM_NAME%"=="1" (
    set "CUSTOM_NAME="
    set /p "CUSTOM_NAME=Enter version name (press Enter to skip): "
    call :trim_value CUSTOM_NAME
    if "!CUSTOM_NAME!"=="" (
        echo No custom name entered
    ) else (
        echo Using name: !CUSTOM_NAME!
    )
    echo.
)

for /f %%i in ('powershell -command "Get-Date -Format yyyyMMddHHmmss"') do set "datetime=%%i"

set "target_dir=..\..\..\builds\pfc\!datetime!"
if defined CUSTOM_NAME set "target_dir=!target_dir!_!CUSTOM_NAME!"

set "bin_name=!datetime!.bin"
if defined CUSTOM_NAME set "bin_name=!CUSTOM_NAME!_!datetime!.bin"

echo Build info:
if defined CUSTOM_NAME (
    echo   Name: !CUSTOM_NAME!
) else (
    echo   Name: date-only
)
echo   Timestamp: !datetime!
echo   Output dir: !target_dir!
echo   Firmware file: !bin_name!
echo.

if /I "%BUILD_MODE%"=="rebuild" (
    echo Step 1: clean build directory...
    call :clean_build_dir
    if errorlevel 1 goto error
) else (
    echo Step 1: skip clean build directory...
)

echo Step 2: create output directory...
if not exist "!target_dir!" mkdir "!target_dir!"
if errorlevel 1 (
    echo Error: failed to create output directory.
    goto error
)
echo Output directory ready.

echo Step 3: build fw_info tool...
if exist fw_info.exe del /f /q fw_info.exe
gcc -DFIRMWARE_TYPE_ISP -o fw_info.exe fw_info.c -DIS_PFC
if errorlevel 1 (
    echo Error: failed to build fw_info.exe.
    goto error
)
echo fw_info.exe built successfully.

echo Step 4: build firmware...
mingw32-make.exe -s -j10 OUTPUT_NAME=%OUTPUT_NAME%
if errorlevel 1 (
    echo Error: firmware build failed.
    goto error
)
echo Firmware build completed.

set "BUILD_BIN=.\build\demo.bin"
if not exist "%BUILD_BIN%" (
    echo Error: expected output file not found: %BUILD_BIN%
    goto error
)

echo Step 5: collect Git info...
set "GIT_COMMIT=unknown"
set "GIT_BRANCH=unknown"
set "GIT_STATUS=clean"

for /f "delims=" %%i in ('git rev-parse --verify --short=16 HEAD 2^>nul') do set "GIT_COMMIT=%%i"
for /f "delims=" %%i in ('git branch --show-current 2^>nul') do set "GIT_BRANCH=%%i"
git diff-index --quiet HEAD -- 2>nul
if errorlevel 1 set "GIT_STATUS=dirty"
echo Git info collected.

echo Step 6: write Git report...
(
echo ========================================
echo Git Detailed Report
echo ========================================
echo.
if defined CUSTOM_NAME (
echo Name: !CUSTOM_NAME!
) else (
echo Name: date-only
)
echo Timestamp: !datetime!
echo Build mode: %BUILD_MODE_NAME%
echo.
echo Repo status: !GIT_STATUS!
echo Branch: !GIT_BRANCH!
echo Commit: !GIT_COMMIT!
echo.
) > "!target_dir!\git_detailed_report.txt"

git --no-pager log -5 --pretty=format:"Commit: %%H%%nAuthor: %%an <%%ae>%%nDate: %%ad%%nSubject: %%s%%n" --date=short >> "!target_dir!\git_detailed_report.txt" 2>nul
echo.>> "!target_dir!\git_detailed_report.txt"
git --no-pager log -3 -p --pretty=format:"%%n=== Commit: %%H ===%%nAuthor: %%an%%nDate: %%ad%%nSubject: %%s%%n" --date=short >> "!target_dir!\git_detailed_report.txt" 2>nul
echo Git report written.

echo Step 7: append footer...
fw_info.exe "%BUILD_BIN%" "!target_dir!\!bin_name!" "!GIT_COMMIT!"
if errorlevel 1 (
    echo Error: failed to append footer.
    goto error
)
echo Footer appended successfully.

echo Step 8: write build report...
(
echo Build Report
echo ============
echo.
if defined CUSTOM_NAME (
echo Name: !CUSTOM_NAME!
) else (
echo Name: date-only
)
echo Timestamp: !datetime!
echo Build mode: %BUILD_MODE_NAME%
echo Firmware type: ISP
echo Platform macro: IS_DC
echo.
echo Git:
echo   Commit: !GIT_COMMIT!
echo   Branch: !GIT_BRANCH!
echo   Status: !GIT_STATUS!
echo.
echo Output:
echo   !bin_name!
echo.
echo Location:
echo   !target_dir!\!bin_name!
) > "!target_dir!\build_report.txt"

echo.
echo Build result:
echo   Output dir: !target_dir!
echo   Firmware: !bin_name!
echo   Build report: build_report.txt
echo   Git report: git_detailed_report.txt
echo   Debug ELF: build\demo.elf
for %%I in ("!target_dir!\!bin_name!") do echo   File size: %%~zI bytes
echo.
echo Build finished successfully.
goto end

:clean_only
echo Step 1: clean build directory...
call :clean_build_dir
if errorlevel 1 goto error
echo.
echo Clean completed successfully.
echo Clean dir: build
goto end

:error
echo.
echo Build failed.
exit /b 1

:parse_args
if "%~1"=="" exit /b 0

if /I "%~1"=="ISP" (
    shift
    goto parse_args
)

if /I "%~1"=="-r" (
    set "BUILD_MODE=rebuild"
    set "BUILD_MODE_NAME=Rebuild"
    shift
    goto parse_args
)

if /I "%~1"=="-c" (
    set "BUILD_MODE=clean"
    set "BUILD_MODE_NAME=Clean Only"
    shift
    goto parse_args
)

if /I "%~1"=="-b" (
    set "BUILD_MODE=build"
    set "BUILD_MODE_NAME=Incremental Build"
    shift
    goto parse_args
)

if /I "%~1"=="-n" (
    set "BUILD_MODE=rebuild"
    set "BUILD_MODE_NAME=Named Rebuild"
    set "PROMPT_CUSTOM_NAME=1"
    shift
    goto parse_args
)

if /I "%~1"=="-h" goto show_help
if /I "%~1"=="/?" goto show_help

if not defined CUSTOM_NAME (
    set "CUSTOM_NAME=%~1"
    shift
    goto parse_args
)

echo Error: unsupported argument "%~1"
exit /b 1

:show_help
echo Usage:
echo   compile.bat [-r^|-c^|-b^|-n] [name]
echo.
echo Options:
echo   -r    clean build directory and then build
echo   -c    only clean build directory
echo   -b    incremental build
echo   -n    clean build, then prompt for version name
echo.
echo Examples:
echo   compile.bat
echo   compile.bat -r
echo   compile.bat demo_v1
echo   compile.bat -n
exit /b 2

:clean_build_dir
if exist "build" rmdir /s /q "build"
exit /b 0

:trim_value
setlocal EnableDelayedExpansion
set "value=!%~1!"
for /f "tokens=* delims= " %%A in ("!value!") do set "value=%%A"
:trim_value_loop
if defined value if "!value:~-1!"==" " (
    set "value=!value:~0,-1!"
    goto trim_value_loop
)
endlocal & set "%~1=%value%"
exit /b 0

:end
endlocal
