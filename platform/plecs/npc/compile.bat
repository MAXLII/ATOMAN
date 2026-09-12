@echo off
setlocal

rem Run from any working directory; use the same x64 toolchain as the NPC build.
set "NPC_PROJECT_DIR=%~dp0."
set "NPC_BUILD_DIR=%NPC_PROJECT_DIR%\build"
set "NPC_COMPILER_DIR=C:\mingw64\bin"
set "PATH=%NPC_COMPILER_DIR%;%PATH%"

if not "%~1"=="" if /I not "%~1"=="test" goto usage
if not "%~2"=="" goto usage

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not on PATH.
    exit /b 1
)
if not exist "%NPC_COMPILER_DIR%\gcc.exe" (
    echo ERROR: Missing compiler: %NPC_COMPILER_DIR%\gcc.exe
    exit /b 1
)
if not exist "%NPC_COMPILER_DIR%\mingw32-make.exe" (
    echo ERROR: Missing build tool: %NPC_COMPILER_DIR%\mingw32-make.exe
    exit /b 1
)

echo Configuring NPC Release build...
cmake -S "%NPC_PROJECT_DIR%" -B "%NPC_BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_C_COMPILER="%NPC_COMPILER_DIR%/gcc.exe" -DCMAKE_MAKE_PROGRAM="%NPC_COMPILER_DIR%/mingw32-make.exe" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1

echo Building NPC DLL...
cmake --build "%NPC_BUILD_DIR%" --target plecs_npc --parallel 4
if errorlevel 1 exit /b 1

if /I "%~1"=="test" (
    where ctest >nul 2>&1
    if errorlevel 1 (
        echo ERROR: CTest is not on PATH.
        exit /b 1
    )
    cmake --build "%NPC_BUILD_DIR%" --target npc_pwm_test --parallel 4
    if errorlevel 1 exit /b 1
    ctest --test-dir "%NPC_BUILD_DIR%" --output-on-failure
    if errorlevel 1 exit /b 1
)

if not exist "%NPC_BUILD_DIR%\bin\plecs_npc.dll" (
    echo ERROR: Build finished without the expected DLL.
    exit /b 1
)
echo.
echo Build successful: %NPC_BUILD_DIR%\bin\plecs_npc.dll
exit /b 0

:usage
echo Usage: compile.bat [test]
echo   compile.bat       Build the NPC DLL.
echo   compile.bat test  Build the DLL and run the integration tests.
exit /b 2
