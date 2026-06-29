@echo off
chcp 65001 >nul
setlocal

set "PORT=%~1"
set "BAUD=%~2"
if "%PORT%"=="" set "PORT=COM8"
if "%BAUD%"=="" set "BAUD=921600"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$p=New-Object System.IO.Ports.SerialPort('%PORT%',%BAUD%,'None',8,'One');" ^
  "try{$p.Open(); Write-Host '%PORT% open ok at %BAUD%'; $p.Close(); exit 0}" ^
  "catch{Write-Host ('%PORT% open failed: ' + $_.Exception.Message); exit 1}"
