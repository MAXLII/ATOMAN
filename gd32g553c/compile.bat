@echo off
chcp 65001 >nul
title 智能固件构建工具

echo 🏗️ ========================================
echo       智能固件构建工具
echo 🏗️ ========================================
echo.

setlocal enabledelayedexpansion

set /p BUILD_TYPE=请输入构建类型 (1=IAP, 2=ISP): 

if "%BUILD_TYPE%"=="1" (
    echo 构建 IAP 版本...
    set FW_TYPE=IAP
    set OUTPUT_NAME=iap_firmware
    set FW_TYPE_DEFINE=-DFIRMWARE_TYPE_IAP
) else if "%BUILD_TYPE%"=="2" (
    echo 构建 ISP 版本...
    set FW_TYPE=ISP
    set OUTPUT_NAME=isp_firmware
    set FW_TYPE_DEFINE=-DFIRMWARE_TYPE_ISP
) else (
    echo 错误: 无效的构建类型
    exit /b 1
)

echo.
echo 构建配置:
echo   类型: %FW_TYPE%
echo.

:: 1. 输入自定义名称
set /p "custom_name=Enter version name (press Enter to skip): "
if "%custom_name%"=="" (echo No custom name entered) else echo Using name: %custom_name%
echo.

:: 2. 使用 PowerShell 获取标准日期时间
for /f %%i in ('powershell -command "Get-Date -Format 'yyyyMMddHHmmss'"') do set "datetime=%%i"

:: 3. 设置目标目录
set "target_dir=..\..\..\builds\pfc\!datetime!"
if defined custom_name set "target_dir=!target_dir!_!custom_name!"

:: 4. 设置文件名（与目录名保持一致）
set "bin_name=!datetime!.bin"
if defined custom_name set "bin_name=!custom_name!_!datetime!.bin"

echo 📊 构建信息:
if defined custom_name (
    echo   项目名称: !custom_name!
) else (
    echo   项目名称: 纯日期模式
)
echo   构建时间: !datetime!
echo   输出目录: !target_dir!
echo   固件文件: !bin_name!
echo.

:: 5. 创建输出目录
echo 🔄 步骤1: 创建输出目录...
if not exist "!target_dir!" mkdir "!target_dir!"
if !errorlevel! neq 0 (
    echo ❌ 创建目录失败!
    goto error
)
echo ✅ 目录创建成功: !target_dir!

:: 6. 编译fw_info工具
echo 🔄 步骤2: 编译fw_info工具...
if exist fw_info.exe del fw_info.exe
gcc %FW_TYPE_DEFINE% -o fw_info.exe fw_info.c -DIS_PFC
if !errorlevel! neq 0 (
    echo ❌ fw_info编译失败!
    goto error
)
echo ✅ fw_info工具编译成功

:: 7. 编译固件（这里替换为你的实际编译命令）
echo 🔄 步骤3: 编译固件...
echo ⚠️  执行固件编译命令...


:: Delete old build directory
echo Deleting old build directory...
rmdir /s /q "build"

:: Run make to compile the project
echo Running make to configure and build the project...
mingw32-make.exe -s -j10 FW_TYPE=%FW_TYPE% OUTPUT_NAME=%OUTPUT_NAME% DEFINES="%FW_TYPE_DEFINE%"

:: Check if the build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo ✅ 固件编译完成

:: 8. 获取Git信息
echo 🔄 步骤4: 获取Git信息...
set GIT_COMMIT=unknown
set GIT_BRANCH=unknown
set GIT_STATUS=clean

:: 获取commit ID (16位)
git rev-parse --short=16 HEAD >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%i in ('git rev-parse --short=16 HEAD') do set GIT_COMMIT=%%i
) else (
    for /f "tokens=*" %%i in ('git rev-parse HEAD') do set GIT_COMMIT=%%i
    set GIT_COMMIT=!GIT_COMMIT:~0,16!
)

:: 获取分支名称
git branch --show-current >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%i in ('git branch --show-current') do set GIT_BRANCH=%%i
)

:: 检查是否有未提交的更改
git diff-index --quiet HEAD --
if !errorlevel! neq 0 set GIT_STATUS=dirty

echo ✅ Git信息获取完成

:: 9. 生成详细Git报告
echo 🔄 步骤5: 生成Git详细报告...
(
echo ========================================
echo           Git详细变更报告
echo ========================================
echo.
if defined custom_name (
    echo 项目名称: !custom_name!
) else (
    echo 项目名称: 纯日期模式
)
echo 构建时间: !datetime!
echo.
echo 仓库状态: !GIT_STATUS!
echo 当前分支: !GIT_BRANCH!
echo 最新提交: !GIT_COMMIT!
echo.
) > "!target_dir!\git_detailed_report.txt"

:: 添加提交历史和详细差异
git log -5 --pretty=format:"提交ID: %%H%%n作者: %%an <%%ae>%%n日期: %%ad%%n描述: %%s%%n" --date=short >> "!target_dir!\git_detailed_report.txt"
echo. >> "!target_dir!\git_detailed_report.txt"
git log -3 -p --pretty=format:"%%n=== 提交: %%H ===%%n作者: %%an%%n日期: %%ad%%n描述: %%s%%n" --date=short >> "!target_dir!\git_detailed_report.txt"

echo ✅ Git报告生成完成

:: 10. 添加文件尾信息
echo 🔄 步骤6: 添加文件尾信息...
fw_info.exe .\build\test.bin "!target_dir!\!bin_name!"
if !errorlevel! neq 0 (
    echo ❌ 添加文件尾信息失败!
    goto error
)
echo ✅ 文件尾信息添加成功

:: 11. 生成构建报告
echo 🔄 步骤7: 生成构建报告...
(
echo 构建报告
echo ===========
echo.
if defined custom_name (
    echo 项目名称: !custom_name!
) else (
    echo 项目名称: 纯日期模式
)
echo 构建时间: !datetime!
echo.
echo Git信息:
echo   提交: !GIT_COMMIT!
echo   分支: !GIT_BRANCH!
echo   状态: !GIT_STATUS!
echo.
echo 输出文件:
echo   !bin_name!
echo.
echo 文件位置:
echo   !target_dir!\!bin_name!
) > "!target_dir!\build_report.txt"

echo ✅ 构建报告生成完成

:: 12. 显示最终结果
echo.
echo 📊 构建结果:
echo   📁 输出目录: !target_dir!
echo   📄 固件文件: !bin_name!
echo   📋 构建报告: build_report.txt
echo   📝 Git报告: git_detailed_report.txt
echo.
echo   🔍 Git状态: !GIT_COMMIT! [!GIT_BRANCH!] - !GIT_STATUS!

:: 13. 计算文件大小
for %%I in ("!target_dir!\!bin_name!") do (
    echo   📏 固件大小: %%~zI 字节
)

echo.
echo 🎉 ========================================
echo       构建成功完成! ✓
echo 🎉 ========================================

goto end

:error
echo.
echo ❌ ========================================
echo       构建过程出现错误! ✗
echo ❌ ========================================
exit /b 1

:end
endlocal
