@echo off
setlocal

set "ROOT=%~dp0"
pushd "%ROOT%" || exit /b 1

echo Cleaning local build artifacts under:
echo   %CD%
echo.

call :remove_dir "build"
call :remove_dir ".metadata"
call :remove_dir "gd32g553c\build"
call :remove_dir "gd32g553c\builds"
call :remove_dir "gd32g553c\gcc_build"
call :remove_dir "gd32g553c\gd_dbg"
call :remove_dir "gd32g553c\-p"
call :remove_dir "apm32\build"
call :remove_dir "hc32f334\keil_flash"
call :remove_dir "matlab\inv\build"
call :remove_dir "matlab\inv\slprj"

call :remove_known_output_dirs

call :remove_file "gd32g553c\debug_log.txt"
call :remove_file "gd32g553c\fw_info.exe"
call :remove_file "gd32g553c\tools\fw_info\fw_info"
call :remove_file "gd32g553c\tools\fw_info\fw_info.exe"
call :remove_file "plecs\plecs_log_file_path.c"
call :remove_file "plecs\plecs_log.txt"
call :remove_file "plecs\ac\plecs_log_file_path.c"
call :remove_file "_final.py"

for /r %%F in (*.o *.d *.crf *.axf *.hex *.lnp *.map *.dep *.build_log.htm *.log *.slxc *.mexw64 *plecs.autosave JLinkLog.txt EventRecorderStub.scvd) do (
    if exist "%%F" (
        echo Removing file: %%F
        del /f /q "%%F"
    )
)

for /f "delims=" %%F in ('dir /b /s "gd32g553c\gcc\Makefile.tmp.*" 2^>nul') do (
    echo Removing file: %%F
    del /f /q "%%F"
)

echo.
echo Clean complete.
popd
exit /b 0

:remove_dir
if exist "%~1" (
    echo Removing directory: %~1
    rmdir /s /q "%~1"
)
exit /b 0

:remove_file
if exist "%~1" (
    echo Removing file: %~1
    del /f /q "%~1"
)
exit /b 0

:remove_known_output_dirs
for /d /r %%P in (keil_mdk mdk ac) do (
    call :remove_dir "%%P\output"
    call :remove_dir "%%P\Listings"
    call :remove_dir "%%P\Objects"
)
exit /b 0
