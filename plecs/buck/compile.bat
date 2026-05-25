@echo off
setlocal

set "BUILD_DIR=build"
set "COMPILER_PATH=C:/mingw64/bin"

if exist "%BUILD_DIR%" (
    echo Cleaning build directory...
    rmdir /s /q "%BUILD_DIR%"
)

echo Creating build directory...
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Running CMake...
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=%COMPILER_PATH%/x86_64-w64-mingw32-gcc.exe ..
if errorlevel 1 (
    echo CMake failed!
    exit /b 1
)

echo Running Make...
%COMPILER_PATH%/mingw32-make.exe -j8
if errorlevel 1 (
    echo Make failed!
    exit /b 1
)

echo.
echo Build successful.
endlocal
