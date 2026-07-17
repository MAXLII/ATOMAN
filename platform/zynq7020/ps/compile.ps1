param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [ValidateSet(0, 1)]
    [int]$Srtos = 0
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $projectRoot "..\..\..")).Path
$referenceExample = Get-ChildItem -LiteralPath (Join-Path $repoRoot "docs\zynq7020") `
    -Recurse -Directory -Filter "arm_07_uart_tx_rxeco" | Select-Object -First 1
if ($null -eq $referenceExample)
{
    throw "arm_07_uart_tx_rxeco reference project not found under docs/zynq7020"
}
$referenceRoot = Join-Path $referenceExample.FullName "arm_07_uart_tx_rxeco.sdk"
$referenceBsp = Join-Path $referenceRoot "uart_eco_bsp\ps7_cortexa9_0"
$referenceLinker = Join-Path $referenceRoot "uart_eco\src\lscript.ld"
$buildName = if ($Srtos -eq 1) { "build_srtos" } else { "build" }
$makefileName = if ($Srtos -eq 1) { "Makefile.a9_srtos" } else { "Makefile" }
$buildRoot = Join-Path $projectRoot $buildName
$buildBsp = Join-Path $buildRoot "bsp\ps7_cortexa9_0"
$gccBin = Join-Path $XilinxSdk "gnu\aarch32\nt\gcc-arm-none-eabi\bin"
$makeExe = Join-Path $XilinxSdk "gnuwin\bin\make.exe"

if (!(Test-Path -LiteralPath $referenceBsp))
{
    throw "Reference Xilinx BSP not found: $referenceBsp"
}

if (!(Test-Path -LiteralPath $makeExe))
{
    throw "Xilinx SDK make not found: $makeExe"
}

New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "include") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "lib") | Out-Null
Copy-Item -Path (Join-Path $referenceBsp "include\*") -Destination (Join-Path $buildBsp "include") -Recurse -Force
Copy-Item -Path (Join-Path $referenceBsp "lib\*") -Destination (Join-Path $buildBsp "lib") -Recurse -Force
Copy-Item -LiteralPath $referenceLinker -Destination (Join-Path $buildRoot "lscript.ld") -Force

$env:PATH = "$gccBin;$($env:PATH)"
Push-Location $projectRoot
try
{
    & $makeExe -f $makefileName all
    if ($LASTEXITCODE -ne 0)
    {
        throw "Zynq-7020 build failed with exit code $LASTEXITCODE"
    }
}
finally
{
    Pop-Location
}
