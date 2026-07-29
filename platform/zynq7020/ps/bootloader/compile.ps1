param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $projectRoot "..\..\..\..")).Path
$referenceExample = Get-ChildItem -LiteralPath (Join-Path $repoRoot "references\zynq7020") `
    -Recurse -Directory -Filter "arm_09_read_write_flash" | Select-Object -First 1
if ($null -eq $referenceExample)
{
    throw "arm_09_read_write_flash reference project not found under references/zynq7020"
}
$referenceRoot = Join-Path $referenceExample.FullName "arm_09_read_write_flash.sdk"
$referenceLinker = Join-Path $referenceRoot "ps_read_write_flash\src\lscript.ld"
$buildRoot = Join-Path $projectRoot "build"
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
        throw "Refusing to replace BSP workspace outside the bootloader build directory: $resolvedWorkspace"
    }
    Remove-Item -LiteralPath $resolvedWorkspace -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $bspWorkspace | Out-Null
& $xsct (Join-Path $projectRoot "build_bsp.tcl") $hardwareDefinition $bspWorkspace
$generatedBsp = Join-Path $bspWorkspace "bootloader_bsp\ps7_cortexa9_0"
if (($LASTEXITCODE -ne 0) -or !(Test-Path -LiteralPath (Join-Path $generatedBsp "lib\libxil.a")))
{
    throw "Zynq-7020 bootloader BSP generation failed"
}

New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "include") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $buildBsp "lib") | Out-Null
Copy-Item -Path (Join-Path $generatedBsp "include\*") -Destination (Join-Path $buildBsp "include") -Recurse -Force
Copy-Item -Path (Join-Path $generatedBsp "lib\*") -Destination (Join-Path $buildBsp "lib") -Recurse -Force
Copy-Item -LiteralPath $referenceLinker -Destination (Join-Path $buildRoot "lscript.ld") -Force

$linkerPath = Join-Path $buildRoot "lscript.ld"
$linkerText = [System.IO.File]::ReadAllText($linkerPath)
$linkerText = $linkerText.Replace(
    "ps7_ddr_0 : ORIGIN = 0x100000, LENGTH = 0x1FF00000",
    "ps7_ddr_0 : ORIGIN = 0x04000000, LENGTH = 0x1BF00000")
[System.IO.File]::WriteAllText($linkerPath, $linkerText, [System.Text.UTF8Encoding]::new($false))

$env:PATH = "$gccBin;$($env:PATH)"
Push-Location $projectRoot
try
{
    & $makeExe clean
    if ($LASTEXITCODE -ne 0)
    {
        throw "Zynq-7020 bootloader clean failed with exit code $LASTEXITCODE"
    }
    & $makeExe all
    if ($LASTEXITCODE -ne 0)
    {
        throw "Zynq-7020 bootloader build failed with exit code $LASTEXITCODE"
    }
}
finally
{
    Pop-Location
}
