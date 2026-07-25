param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [string]$FrameRoot = "D:\OneDrive\LWX\FRAME",
    [string]$Port = "COM6",
    [int]$Baud = 921600,
    [ValidateSet(0, 1)]
    [int]$Srtos = 0
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$platformRoot = Split-Path -Parent $projectRoot
$downloadScript = Join-Path $projectRoot "download.ps1"
$plSelfTestScript = Join-Path $platformRoot "pl\pl_selftest.ps1"
$frame = Join-Path $FrameRoot "frame.ps1"
$iirCommand = [string]::Concat("IIR_TEST", [char]13, [char]10)
$statusCommand = [string]::Concat("ZYNQ_STATUS", [char]13, [char]10)

foreach ($requiredFile in @($downloadScript, $plSelfTestScript, $frame))
{
    if (!(Test-Path -LiteralPath $requiredFile))
    {
        throw "Required test tool not found: $requiredFile"
    }
}

& $downloadScript -XilinxSdk $XilinxSdk -Srtos $Srtos
Start-Sleep -Milliseconds 3000

& $plSelfTestScript -XilinxSdk $XilinxSdk
Start-Sleep -Milliseconds 500

$savedErrorAction = $ErrorActionPreference
try
{
    $ErrorActionPreference = "Continue"
    $iirOutput = @(& $frame serial raw --port $Port --baud $Baud `
        --send-text $iirCommand --read-seconds 3 2>&1 |
        ForEach-Object { $_.ToString() })
    $iirExitCode = $LASTEXITCODE
}
finally
{
    $ErrorActionPreference = $savedErrorAction
}
$iirOutput | ForEach-Object { Write-Output $_ }
$iirPassed = @($iirOutput | Select-String -SimpleMatch "iir result=PASS").Count -ge 1
if (($iirExitCode -ne 0) -or !$iirPassed)
{
    throw "$Port IIR self-test failed: exit=$iirExitCode pass_marker=$iirPassed"
}

try
{
    $ErrorActionPreference = "Continue"
    $statusOutput = @(& $frame serial raw --port $Port --baud $Baud `
        --send-text $statusCommand --read-seconds 2 2>&1 |
        ForEach-Object { $_.ToString() })
    $statusExitCode = $LASTEXITCODE
}
finally
{
    $ErrorActionPreference = $savedErrorAction
}
$statusOutput | ForEach-Object { Write-Output $_ }
$expectedMode = if ($Srtos -eq 1) { "srtos-a9" } else { "baremetal" }
$modePassed = @($statusOutput | Select-String -SimpleMatch "zynq mode=$expectedMode").Count -ge 1
if (($statusExitCode -ne 0) -or !$modePassed)
{
    throw "$Port $expectedMode status check failed: exit=$statusExitCode pass_marker=$modePassed"
}

Write-Output "BOARD_IIR_SELFTEST result=PASS port=$Port baud=$Baud mode=$expectedMode"
