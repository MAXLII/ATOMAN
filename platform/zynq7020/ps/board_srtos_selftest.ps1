param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [string]$FrameRoot = "D:\OneDrive\LWX\FRAME",
    [string]$Port = "COM5",
    [int]$Baud = 115200
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$compileScript = Join-Path $projectRoot "compile.ps1"
$downloadScript = Join-Path $projectRoot "download.ps1"
$frame = Join-Path $FrameRoot "frame.ps1"
$combinedCommand = [string]::Concat("ZYNQ_STATUS", [char]13, [char]10,
                                    "IIR_TEST", [char]13, [char]10)
$statusCommand = [string]::Concat("ZYNQ_STATUS", [char]13, [char]10)
$statusPattern = "zynq mode=srtos-a9 tick_100us=(\d+) task_count=(\d+) section=[0-9A-F]+-[0-9A-F]+ srtos=1 fault=0 save_fail=0 release_fail=0 pool=(\d+)/(\d+) stack_free=(\d+)"

foreach ($requiredFile in @($compileScript, $downloadScript, $frame))
{
    if (!(Test-Path -LiteralPath $requiredFile))
    {
        throw "Required SRTOS test tool not found: $requiredFile"
    }
}

& $compileScript -XilinxSdk $XilinxSdk -Srtos 1
& $downloadScript -XilinxSdk $XilinxSdk -Srtos 1
Start-Sleep -Milliseconds 2500

$savedErrorAction = $ErrorActionPreference
try
{
    $ErrorActionPreference = "Continue"
    $firstOutput = @(& $frame serial raw --port $Port --baud $Baud `
        --send-text $combinedCommand --read-seconds 4 2>&1 |
        ForEach-Object { $_.ToString() })
    $firstExitCode = $LASTEXITCODE
}
finally
{
    $ErrorActionPreference = $savedErrorAction
}
$firstOutput | ForEach-Object { Write-Output $_ }
if ($firstExitCode -ne 0)
{
    throw "FRAME SRTOS probe failed: exit=$firstExitCode"
}

$firstStatusLine = @($firstOutput | Where-Object { $_ -match "zynq mode=srtos-a9" } | Select-Object -Last 1)
if ($firstStatusLine.Count -ne 1)
{
    throw "SRTOS status response was not received"
}
$firstStatusMatch = [regex]::Match($firstStatusLine[0], $statusPattern)
if (!$firstStatusMatch.Success)
{
    throw "SRTOS status contains a scheduler fault: $($firstStatusLine[0])"
}
if (@($firstOutput | Select-String -SimpleMatch "iir result=PASS").Count -lt 1)
{
    throw "SRTOS IIR self-test did not pass"
}
if (@($firstOutput | Select-String -SimpleMatch "TASK_DEAD_LOOP_100MS").Count -lt 1)
{
    throw "100 ms preempted long task produced no output"
}
if (@($firstOutput | Select-String -SimpleMatch "TASK_DEAD_LOOP_123MS").Count -lt 1)
{
    throw "123 ms preempted long task produced no output"
}
$firstTaskCount = [uint64]$firstStatusMatch.Groups[2].Value

Start-Sleep -Milliseconds 1500
try
{
    $ErrorActionPreference = "Continue"
    $secondOutput = @(& $frame serial raw --port $Port --baud $Baud `
        --send-text $statusCommand --read-seconds 2 2>&1 |
        ForEach-Object { $_.ToString() })
    $secondExitCode = $LASTEXITCODE
}
finally
{
    $ErrorActionPreference = $savedErrorAction
}
$secondOutput | ForEach-Object { Write-Output $_ }
if ($secondExitCode -ne 0)
{
    throw "FRAME SRTOS follow-up probe failed: exit=$secondExitCode"
}

$secondStatusLine = @($secondOutput | Where-Object { $_ -match "zynq mode=srtos-a9" } | Select-Object -Last 1)
if ($secondStatusLine.Count -ne 1)
{
    throw "SRTOS follow-up status response was not received"
}
$secondStatusMatch = [regex]::Match($secondStatusLine[0], $statusPattern)
if (!$secondStatusMatch.Success)
{
    throw "SRTOS follow-up status contains a scheduler fault: $($secondStatusLine[0])"
}
$secondTaskCount = [uint64]$secondStatusMatch.Groups[2].Value
if ($secondTaskCount -le $firstTaskCount)
{
    throw "SRTOS platform task did not advance: first=$firstTaskCount second=$secondTaskCount"
}

Write-Output "BOARD_SRTOS_SELFTEST result=PASS port=$Port baud=$Baud task_count=$firstTaskCount->$secondTaskCount fault=0 save_fail=0 release_fail=0 iir=PASS"
