@echo off
set "UV4_EXE="

for %%P in (
    "%LocalAppData%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles%\Keil_v5\UV4\UV4.exe"
    "%ProgramFiles(x86)%\Keil_v5\UV4\UV4.exe"
) do (
    if exist %%~P (
        set "UV4_EXE=%%~fP"
        goto found_uv4
    )
)

for /f "delims=" %%I in ('where UV4.exe 2^>nul') do (
    set "UV4_EXE=%%~fI"
    goto found_uv4
)

echo ERROR: UV4.exe not found.
exit /b 1

:found_uv4
exit /b 0
