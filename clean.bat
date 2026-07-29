@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
pushd "%PROJECT_ROOT%" || exit /b 1

echo Cleaning generated files under:
echo   %CD%
echo.

for %%D in (
    "build"
    ".Xil"
    "xsim.dir"
    "tmp"
    "platform\apm32\build"
    "platform\gd32g553c\build"
    "platform\gd32g553c\builds"
    "platform\gd32g553c\gcc_build"
    "platform\gd32g553c\gd_dbg"
    "platform\gd32g553c\-p"
    "platform\gd32g553c\mdk\Listings"
    "platform\gd32g553c\mdk\Objects"
    "platform\hc32f334\gcc\build"
    "platform\hc32f334\bootloader\gcc\build"
    "platform\hc32f334\keil_flash"
    "platform\hc32f334\keil_mdk\output"
    "platform\hc32f334\keil_mdk\output_bootloader"
    "platform\hc32f334\keil_mdk\output_iap"
    "platform\hc32f334\keil_mdk\output_isp"
    "platform\hc32f558\keil_mdk\output"
    "platform\matlab\inv\build"
    "platform\matlab\inv\slprj"
    "platform\matlab\pll\build"
    "platform\matlab\pll\slprj"
    "platform\zynq7020\pl\.Xil"
    "platform\zynq7020\pl\NA"
    "platform\zynq7020\pl\build"
    "platform\zynq7020\ps\.Xil"
    "platform\zynq7020\ps\build"
    "platform\zynq7020\ps\build_srtos"
    "platform\zynq7020\ps\build_srtos_stress"
    "platform\zynq7020\ps\bootloader\build"
    "platform\zynq7020\ps\test\build"
) do (
    if exist "%%~D" (
        echo Removing directory: %%~D
        rmdir /s /q "%%~D" 2>nul
        if exist "%%~D" echo [WARN] Directory is still in use: %%~D
    )
)

for /d %%P in ("platform\plecs\*") do (
    if exist "%%~fP\build" (
        echo Removing directory: %%~fP\build
        rmdir /s /q "%%~fP\build" 2>nul
        if exist "%%~fP\build" echo [WARN] Directory is still in use: %%~fP\build
    )
    for /d %%B in ("%%~fP\build_*") do (
        echo Removing directory: %%~fB
        rmdir /s /q "%%~fB" 2>nul
        if exist "%%~fB" echo [WARN] Directory is still in use: %%~fB
    )
)

for /d %%P in ("verilog\*") do (
    if exist "%%~fP\sim\.Xil" (
        echo Removing directory: %%~fP\sim\.Xil
        rmdir /s /q "%%~fP\sim\.Xil" 2>nul
        if exist "%%~fP\sim\.Xil" echo [WARN] Directory is still in use: %%~fP\sim\.Xil
    )
)

for %%F in (vivado.log vivado.jou xvlog.log xvlog.pb _final.py) do (
    if exist "%%~F" (
        echo Removing file: %%~F
        del /f /q "%%~F" 2>nul
    )
)

for %%R in (platform tests verilog) do (
    if exist "%%~R" for /r "%%~R" %%F in (*.o *.d *.dep *.crf *.axf *.elf *.hex *.map *.lnp *.build_log.htm *.log *.mexw64 *.slxc *.plecs.autosave JLinkLog.txt EventRecorderStub.scvd) do (
        if exist "%%~fF" (
            echo Removing file: %%~fF
            del /f /q "%%~fF" 2>nul
            if exist "%%~fF" echo [WARN] File is still in use: %%~fF
        )
    )
)

if exist "tests" for /r "tests" %%F in (*.exe) do (
    if exist "%%~fF" (
        echo Removing file: %%~fF
        del /f /q "%%~fF" 2>nul
    )
)

for /f "delims=" %%F in ('dir /b /s "platform\gd32g553c\gcc\Makefile.tmp.*" 2^>nul') do (
    echo Removing file: %%~fF
    del /f /q "%%~fF" 2>nul
)

echo.
echo Clean complete.
popd
exit /b 0
