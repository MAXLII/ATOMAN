@echo off
setlocal

set "REPO_ROOT=%~dp0"
for %%I in ("%REPO_ROOT%.") do set "REPO_ROOT=%%~fI"

echo [1/2] Checking unstaged diff whitespace...
git -c core.safecrlf=false -C "%REPO_ROOT%" diff --check
if errorlevel 1 exit /b 1

echo [2/2] Checking staged diff whitespace...
git -c core.safecrlf=false -C "%REPO_ROOT%" diff --cached --check
if errorlevel 1 exit /b 1

echo Repository checks passed.
exit /b 0
