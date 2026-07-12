@echo off
for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
set /A "start_time=(((%%a*60)+%%b)*60)+%%c"
)

set BUILD_DIR=./build
set COMPILER_PATH=C:/mingw64/bin
set CMAKE_COMMAND=cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=%COMPILER_PATH%/x86_64-w64-mingw32-gcc.exe -DCMAKE_CXX_COMPILER=%COMPILER_PATH%/x86_64-w64-mingw32-g++.exe ..
set MAKE_COMMAND=%COMPILER_PATH%/mingw32-make.exe -j8

if exist "%BUILD_DIR%" (
echo Cleaning build directory...
rmdir /s /q "%BUILD_DIR%"
)

echo Creating build directory...
mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

echo Running CMake...
%CMAKE_COMMAND%
if errorlevel 1 (
echo CMake failed!
exit /b 1
)

echo Creating bin directory...
if not exist "bin" mkdir bin

echo Running Make...
%MAKE_COMMAND%
if errorlevel 1 (
echo Make failed!
exit /b 1
)

for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
set /A "end_time=(((%%a*60)+%%b)*60)+%%c"
)

set /A "elapsed_time=end_time - start_time"
set /A "elapsed_h=elapsed_time / 3600"
set /A "elapsed_m=(elapsed_time %% 3600) / 60"
set /A "elapsed_s=elapsed_time %% 60"

echo.
echo ===== Build successful! =====
echo Total time: %elapsed_h%h %elapsed_m%m %elapsed_s%s
