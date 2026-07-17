param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$xsct = Join-Path $XilinxSdk "bin\xsct.bat"
$selfTestScript = Join-Path $projectRoot "pl_selftest.tcl"

if (!(Test-Path -LiteralPath $xsct))
{
    throw "Xilinx SDK xsct not found: $xsct"
}

Push-Location $projectRoot
try
{
    $savedErrorAction = $ErrorActionPreference
    try
    {
        $ErrorActionPreference = "Continue"
        $xsctOutput = @(& $xsct $selfTestScript 2>&1 |
            ForEach-Object { $_.ToString() })
        $xsctExitCode = $LASTEXITCODE
    }
    finally
    {
        $ErrorActionPreference = $savedErrorAction
    }
    $xsctOutput | ForEach-Object { Write-Output $_ }
    $selfTestPassed = @($xsctOutput | Select-String -SimpleMatch "PL_SELFTEST result=PASS").Count -eq 1
    if (($xsctExitCode -ne 0) -or !$selfTestPassed)
    {
        throw "PL AXI self-test failed: exit=$xsctExitCode pass_marker=$selfTestPassed"
    }
}
finally
{
    Pop-Location
}
