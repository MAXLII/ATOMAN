@echo off
setlocal
cd /d "%~dp0"
set "MODE=srtos_m"
if /I "%~1"=="baremetal" set "MODE=baremetal"
if /I "%~1"=="srtos_m" set "MODE=srtos_m"
if not "%~1"=="" if /I not "%~1"=="baremetal" if /I not "%~1"=="srtos_m" exit /b 2
if not defined JLINK_EXE set "JLINK_EXE=C:\Program Files\SEGGER\JLink_V936\JLink.exe"
if not exist "build_%MODE%\stm32f103c8t6_demo.bin" exit /b 1
> "build_%MODE%\download.jlink" echo r
>> "build_%MODE%\download.jlink" echo h
>> "build_%MODE%\download.jlink" echo loadbin build_%MODE%/stm32f103c8t6_demo.bin, 0x08000000
>> "build_%MODE%\download.jlink" echo verifybin build_%MODE%/stm32f103c8t6_demo.bin, 0x08000000
>> "build_%MODE%\download.jlink" echo r
>> "build_%MODE%\download.jlink" echo g
>> "build_%MODE%\download.jlink" echo q
"%JLINK_EXE%" -NoGui 1 -device STM32F103C8 -if SWD -speed 1000 -autoconnect 1 -ExitOnError 1 -CommandFile "build_%MODE%\download.jlink"
exit /b %errorlevel%
