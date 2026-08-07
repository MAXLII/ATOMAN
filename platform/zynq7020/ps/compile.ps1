param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [ValidateSet(0, 1)]
    [int]$Srtos = 0
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $projectRoot "..\..\..")).Path
$referenceExample = Get-ChildItem -LiteralPath (Join-Path $repoRoot "references\zynq7020") `
    -Recurse -Directory -Filter "arm_07_uart_tx_rxeco" | Select-Object -First 1
if ($null -eq $referenceExample)
{
    throw "arm_07_uart_tx_rxeco reference project not found under references/zynq7020"
}
$referenceRoot = Join-Path $referenceExample.FullName "arm_07_uart_tx_rxeco.sdk"
$referenceLinker = Join-Path $referenceRoot "uart_eco\src\lscript.ld"
$buildName = if ($Srtos -eq 1) { "build_srtos" } else { "build" }
$makefileName = if ($Srtos -eq 1) { "Makefile.a9_srtos" } else { "Makefile" }
$buildRoot = Join-Path $projectRoot $buildName
$buildBsp = Join-Path $buildRoot "bsp\ps7_cortexa9_0"
$bspWorkspace = Join-Path $buildRoot "bsp_workspace"
$hardwareDefinition = Join-Path $repoRoot "platform\zynq7020\pl\build\output\zynq7020_platform.hdf"
$gccBin = Join-Path $XilinxSdk "gnu\aarch32\nt\gcc-arm-none-eabi\bin"
$makeExe = Join-Path $XilinxSdk "gnuwin\bin\make.exe"
$xsct = Join-Path $XilinxSdk "bin\xsct.bat"

if (!(Test-Path -LiteralPath $hardwareDefinition))
{
    throw "Zynq-7020 hardware definition not found: $hardwareDefinition"
}

if (!(Test-Path -LiteralPath $makeExe) -or !(Test-Path -LiteralPath $xsct))
{
    throw "Xilinx SDK build tools not found under: $XilinxSdk"
}

if (Test-Path -LiteralPath $bspWorkspace)
{
    $resolvedWorkspace = (Resolve-Path -LiteralPath $bspWorkspace).Path
    $resolvedBuildRoot = (Resolve-Path -LiteralPath $buildRoot).Path
    if (!$resolvedWorkspace.StartsWith($resolvedBuildRoot + "\", [System.StringComparison]::OrdinalIgnoreCase))
    {
        throw "Refusing to replace BSP workspace outside the application build directory: $resolvedWorkspace"
    }
    Remove-Item -LiteralPath $resolvedWorkspace -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $bspWorkspace | Out-Null
& $xsct (Join-Path $projectRoot "build_bsp.tcl") $hardwareDefinition $bspWorkspace
$generatedBsp = Join-Path $bspWorkspace "application_bsp\ps7_cortexa9_0"
if (($LASTEXITCODE -ne 0) -or
    !(Test-Path -LiteralPath (Join-Path $generatedBsp "lib\libxil.a")) -or
    !(Test-Path -LiteralPath (Join-Path $generatedBsp "lib\liblwip4.a")))
{
    throw "Zynq-7020 application BSP generation failed"
}

New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "include") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "lib") | Out-Null
Copy-Item -Path (Join-Path $generatedBsp "include\*") -Destination (Join-Path $buildBsp "include") -Recurse -Force
Copy-Item -Path (Join-Path $generatedBsp "lib\*") -Destination (Join-Path $buildBsp "lib") -Recurse -Force
Copy-Item -LiteralPath $referenceLinker -Destination (Join-Path $buildRoot "lscript.ld") -Force

$env:PATH = "$gccBin;$($env:PATH)"
Push-Location $projectRoot
try
{
    & $makeExe -f $makefileName clean
    if ($LASTEXITCODE -ne 0)
    {
        throw "Zynq-7020 clean failed with exit code $LASTEXITCODE"
    }
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
