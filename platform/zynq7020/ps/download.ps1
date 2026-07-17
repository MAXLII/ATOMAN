param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [ValidateSet(0, 1)]
    [int]$Srtos = 0
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$platformRoot = Split-Path -Parent $projectRoot
$xsct = Join-Path $XilinxSdk "bin\xsct.bat"
$buildName = if ($Srtos -eq 1) { "build_srtos" } else { "build" }
$firmwareName = if ($Srtos -eq 1) { "zynq7020_section_comm_srtos.elf" } else { "zynq7020_section_comm.elf" }
$firmware = Join-Path (Join-Path $projectRoot $buildName) $firmwareName
$plOutput = Join-Path $platformRoot "pl\build\output"
$plBuildScript = Join-Path $platformRoot "pl\build_pl.ps1"
$plArtifacts = @(
    (Join-Path $plOutput "ps7_init.tcl"),
    (Join-Path $plOutput "zynq7020_platform.bit"),
    (Join-Path $plOutput "zynq7020_platform.hdf")
)

if (!(Test-Path -LiteralPath $firmware))
{
    throw "Firmware not found. Run compile.ps1 first: $firmware"
}

if (!(Test-Path -LiteralPath $xsct))
{
    throw "Xilinx SDK xsct not found: $xsct"
}

foreach ($artifact in $plArtifacts)
{
    if (!(Test-Path -LiteralPath $artifact))
    {
        throw "PL artifact not found. Run $plBuildScript first: $artifact"
    }
}

Push-Location $projectRoot
try
{
    $savedErrorAction = $ErrorActionPreference
    try
    {
        $ErrorActionPreference = "Continue"
        $xsctOutput = @(& $xsct (Join-Path $projectRoot "download.tcl") $Srtos 2>&1 |
            ForEach-Object { $_.ToString() })
        $xsctExitCode = $LASTEXITCODE
    }
    finally
    {
        $ErrorActionPreference = $savedErrorAction
    }
    $xsctOutput | ForEach-Object { Write-Output $_ }
    $downloadPassed = @($xsctOutput | Select-String -SimpleMatch "DOWNLOAD_RESULT status=PASS").Count -eq 1
    if (($xsctExitCode -ne 0) -or !$downloadPassed)
    {
        throw "Zynq-7020 JTAG download failed: exit=$xsctExitCode pass_marker=$downloadPassed"
    }
}
finally
{
    Pop-Location
}
