param(
    [string]$FrameRoot = "D:\OneDrive\LWX\FRAME",
    [string]$DebugPort = "COM6",
    [string]$PlPort = "COM7",
    [int]$Baud = 921600,
    [int]$Frames = 11000
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$python = Join-Path $FrameRoot ".venv\Scripts\python.exe"
$helper = Join-Path $projectRoot "board_pl_uart_dma_stress.py"
$outputRoot = Join-Path $projectRoot "build\pl_uart_dma_acceptance"
$monitorOutput = Join-Path $outputRoot "com6_monitor.log"
$monitorError = Join-Path $outputRoot "com6_monitor_error.log"

foreach ($requiredFile in @($python, $helper))
{
    if (!(Test-Path -LiteralPath $requiredFile))
    {
        throw "Required acceptance file not found: $requiredFile"
    }
}

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
Remove-Item -LiteralPath $monitorOutput, $monitorError -Force -ErrorAction SilentlyContinue

$monitorArguments = @(
    $helper,
    "monitor",
    "--frame-root", $FrameRoot,
    "--port", $DebugPort,
    "--baud", $Baud,
    "--duration", 600
)
$monitor = Start-Process -FilePath $python `
    -ArgumentList $monitorArguments `
    -WindowStyle Hidden `
    -RedirectStandardOutput $monitorOutput `
    -RedirectStandardError $monitorError `
    -PassThru

try
{
    Start-Sleep -Milliseconds 500
    & $python $helper stress `
        --frame-root $FrameRoot `
        --port $PlPort `
        --baud $Baud `
        --frames $Frames
    if ($LASTEXITCODE -ne 0)
    {
        throw "COM7 FRAME stress failed: exit=$LASTEXITCODE"
    }
}
finally
{
    $monitorProcesses = Get-CimInstance Win32_Process | Where-Object {
        ($_.Name -eq "python.exe") -and
        ($_.CommandLine -like "*board_pl_uart_dma_stress.py monitor*") -and
        ($_.CommandLine -like "*--port $DebugPort*")
    }
    foreach ($monitorProcess in $monitorProcesses)
    {
        Stop-Process -Id $monitorProcess.ProcessId -Force -ErrorAction SilentlyContinue
    }
}

$monitorText = Get-Content -LiteralPath $monitorOutput -Encoding UTF8 -Raw
$monitorErrors = Get-Content -LiteralPath $monitorError -Encoding UTF8 -Raw
if (![string]::IsNullOrWhiteSpace($monitorErrors))
{
    throw "COM6 monitor reported an error: $monitorErrors"
}
if ($monitorText -notmatch "pl_uart version=00010000")
{
    throw "COM6 monitor did not capture PL_UART_STATUS"
}
if ($monitorText -match "irq=[0-9A-Fa-f]{8}/[1-9][0-9]*/" -or
    $monitorText -match "uart_err=(?!00000000)" -or
    $monitorText -match "dma_err=(?!00000000)" -or
    $monitorText -match "stop=(?!00000000)")
{
    throw "COM6 monitor captured a PL UART DMA error"
}

$frame = Join-Path $FrameRoot "frame.ps1"
$finalStatus = & $frame serial raw `
    --port $DebugPort `
    --baud $Baud `
    --send-text "PL_UART_STATUS\r\n" `
    --read-seconds 1
if ($LASTEXITCODE -ne 0)
{
    throw "Final COM6 status query failed: exit=$LASTEXITCODE"
}
$statusLine = @($finalStatus | Select-String -Pattern "pl_uart version=")[0].Line
if ($statusLine -notmatch "irq=00000000/0/00000000" -or
    $statusLine -notmatch "uart_err=00000000" -or
    $statusLine -notmatch "dma_err=00000000" -or
    $statusLine -notmatch "stop=00000000")
{
    throw "Final PL UART DMA status is not clean: $statusLine"
}
if ($statusLine -notmatch "rx=(\d+)/(\d+) tx=(\d+)/(\d+)")
{
    throw "Unable to parse final ring counters: $statusLine"
}
if (($Matches[1] -ne $Matches[2]) -or ($Matches[3] -ne $Matches[4]))
{
    throw "Final ring counters are not drained: $statusLine"
}

Write-Output $statusLine
Write-Output "BOARD_PL_UART_DMA_SELFTEST result=PASS debug_port=$DebugPort pl_port=$PlPort baud=$Baud"
