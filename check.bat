@echo off
setlocal

set "REPO_ROOT=%~dp0"
for %%I in ("%REPO_ROOT%.") do set "REPO_ROOT=%%~fI"
set "DOC_CHECK=%REPO_ROOT%\tools\check_docs.ps1"
set "IIR_CHECK=%REPO_ROOT%\tools\check_iir_consistency.ps1"

echo [1/4] Checking Markdown navigation...
if not exist "%DOC_CHECK%" (
    echo Documentation check script not found: "%DOC_CHECK%"
    exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%DOC_CHECK%"
if errorlevel 1 exit /b 1

echo [2/4] Checking IIR documentation consistency...
if not exist "%IIR_CHECK%" (
    echo IIR consistency check script not found: "%IIR_CHECK%"
    exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%IIR_CHECK%"
if errorlevel 1 exit /b 1

echo [3/4] Checking unstaged diff whitespace...
git -c core.safecrlf=false -C "%REPO_ROOT%" diff --check
if errorlevel 1 exit /b 1

echo [4/4] Checking staged diff whitespace...
git -c core.safecrlf=false -C "%REPO_ROOT%" diff --cached --check
if errorlevel 1 exit /b 1

echo Repository checks passed.
exit /b 0
