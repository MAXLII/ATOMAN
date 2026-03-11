@echo off
chcp 65001 > nul
REM ========================================
REM 高级 J-Link 下载脚本
REM ========================================
REM 配置区域 - 根据你的实际环境修改以下变量
REM ========================================

REM 1. J-Link 安装路径
SET JLINK_PATH=C:\Program Files\SEGGER\JLink

REM 2. 目标设备型号 (使用 J-Link 支持的设备名称，例如：STM32F103RC, nRF52832_xxaa, LPC1768)
SET DEVICE=GD32G553RCT6

REM 3. 接口类型 (JTAG / SWD)
SET IF=SWD

REM 4. 调试器速度 (kHz)
SET SPEED=4000

REM 5. Bin文件的默认起始地址 (HEX文件不需要)
SET BIN_ADDR=0x08000000

REM 6. 日志文件名称
SET LOG_FILE=JLink_Flash.log

REM 7. ★★★ 添加固定的文件路径 ★★★
SET FIXED_FIRMWARE_PATH=build\test.bin

REM ========================================
REM 脚本主体 - 通常无需修改以下内容
REM ========================================

setlocal enabledelayedexpansion

REM 设置窗口标题
title J-Link 下载工具 - %DEVICE%

echo.
echo ========================================
echo          J-Link 程序下载工具
echo ========================================
echo 设备: %DEVICE%
echo 接口: %IF%
echo 速度: %SPEED% kHz
echo ========================================
echo.

REM ★★★ 检查是否提供了文件参数，如果没有则使用固定路径 ★★★
if "%~1"=="" (
    echo [信息] 未提供文件参数，使用固定路径: %FIXED_FIRMWARE_PATH%
    set "FILE_PATH=%FIXED_FIRMWARE_PATH%"
    goto :check_fixed_file
)

REM 检查拖拽的文件是否存在
if not exist "%~1" (
    echo [错误] 文件 "%~1" 不存在！
    goto :end
)

REM 获取拖拽的文件信息
set "FILE_PATH=%~f1"
set "FILE_NAME=%~nx1"
set "FILE_EXT=%~x1"
goto :process_file

:check_fixed_file
REM 检查固定文件是否存在
if not exist "%FIXED_FIRMWARE_PATH%" (
    echo [错误] 固定固件文件不存在！
    echo 请检查路径: %FIXED_FIRMWARE_PATH%
    call :show_usage
    goto :end
)

REM 获取固定文件信息
for %%F in ("%FIXED_FIRMWARE_PATH%") do (
    set "FILE_NAME=%%~nxF"
    set "FILE_EXT=%%~xF"
)

:process_file
REM 验证文件格式并构建加载命令
call :get_load_command
if !ERRORLEVEL! neq 0 goto :end

REM 创建J-Link脚本
set "JLINK_SCRIPT=%TEMP%\jlink_script.tmp"
echo device %DEVICE%        >  "%JLINK_SCRIPT%"
echo si %IF%                >> "%JLINK_SCRIPT%"
echo speed %SPEED%          >> "%JLINK_SCRIPT%"
echo halt                   >> "%JLINK_SCRIPT%"
echo r                      >> "%JLINK_SCRIPT%"
echo !LOAD_CMD!             >> "%JLINK_SCRIPT%"
echo r                      >> "%JLINK_SCRIPT%"
echo g                      >> "%JLINK_SCRIPT%"
echo exit                   >> "%JLINK_SCRIPT%"

REM 执行下载
echo.
echo 正在下载: !FILE_NAME!
echo 文件路径: %FILE_PATH%
echo 目标设备: %DEVICE%
echo ========================================
echo.

"%JLINK_PATH%\JLink.exe" -CommanderScript "%JLINK_SCRIPT%" -Log "%LOG_FILE%"

REM 检查执行结果
if !ERRORLEVEL! equ 0 (
    echo.
    echo ========================================
    echo [成功] 程序下载完成
    echo 日志已保存到: %LOG_FILE%
) else (
    echo.
    echo ========================================
    echo [错误] 下载过程中出现错误！
    echo 请检查日志文件: %LOG_FILE%
)

REM 清理临时文件
if exist "%JLINK_SCRIPT%" del "%JLINK_SCRIPT%"

:end
echo.
@REM pause
exit /b

REM ========================================
REM 子程序：显示使用方法
REM ========================================
:show_usage
echo.
echo 使用方法：
echo 1. 直接将 .hex/.bin/.s19 文件拖拽到此批处理文件上
echo 2. 或者：%0 ^<固件文件路径^>
echo 3. 直接双击运行使用固定路径: %FIXED_FIRMWARE_PATH%
echo.
echo 支持的格式：.hex, .bin (需指定地址), .s19
goto :eof

REM ========================================
REM 子程序：根据文件类型构建加载命令
REM ========================================
:get_load_command
if /i "!FILE_EXT!"==".hex" (
    set "LOAD_CMD=loadfile "%FILE_PATH%""
    exit /b 0
)

if /i "!FILE_EXT!"==".bin" (
    echo [信息] 检测到 .bin 文件，使用默认地址: %BIN_ADDR%
    set "LOAD_CMD=loadfile "%FILE_PATH%" %BIN_ADDR%"
    exit /b 0
)

if /i "!FILE_EXT!"==".s19" (
    set "LOAD_CMD=loadfile "%FILE_PATH%""
    exit /b 0
)

echo [错误] 不支持的文件格式: !FILE_EXT!
echo        支持格式: .hex, .bin, .s19
exit /b 1
