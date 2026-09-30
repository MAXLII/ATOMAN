@echo off
setlocal
set "CHB_DIR=%~dp0."
set "BUILD_DIR=%CHB_DIR%\build"
set "MINGW_DIR=C:\mingw64\bin"
set "PATH=%MINGW_DIR%;%PATH%"

cmake -S "%CHB_DIR%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_C_COMPILER="%MINGW_DIR%/gcc.exe" -DCMAKE_MAKE_PROGRAM="%MINGW_DIR%/mingw32-make.exe" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --target plecs_chb plecs_grid_source --parallel 4
if errorlevel 1 exit /b 1
echo Built %BUILD_DIR%\bin\plecs_chb.dll
echo Built %BUILD_DIR%\bin\plecs_grid_source.dll
exit /b 0
