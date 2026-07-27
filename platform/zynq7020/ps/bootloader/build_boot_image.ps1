param(
    [string]$XilinxSdk = "C:\Xilinx\SDK\2018.3",
    [string]$HardwareDefinition = "",
    [string]$Bitstream = "",
    [string]$BootloaderElf = "",
    [string]$FsblElf = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $projectRoot "..\..\..\..")).Path
$buildRoot = Join-Path $projectRoot "build"
$bootgen = Join-Path $XilinxSdk "bin\bootgen.bat"
$xsct = Join-Path $XilinxSdk "bin\xsct.bat"
$bootPartitionSize = 0x00500000
$buildGeneratedFsbl = [string]::IsNullOrWhiteSpace($FsblElf)

if ([string]::IsNullOrWhiteSpace($HardwareDefinition))
{
    $HardwareDefinition = Join-Path $repoRoot "platform\zynq7020\pl\build\output\zynq7020_platform.hdf"
}
if ([string]::IsNullOrWhiteSpace($Bitstream))
{
    $Bitstream = Join-Path $repoRoot "platform\zynq7020\pl\build\output\zynq7020_platform.bit"
}
if ([string]::IsNullOrWhiteSpace($BootloaderElf))
{
    $BootloaderElf = Join-Path $buildRoot "zynq7020_bootloader.elf"
}
if ([string]::IsNullOrWhiteSpace($FsblElf))
{
    $FsblElf = Join-Path $buildRoot "fsbl_workspace\fsbl\Debug\fsbl.elf"
}

foreach ($requiredTool in @($bootgen, $xsct))
{
    if (!(Test-Path -LiteralPath $requiredTool))
    {
        throw "Xilinx SDK tool not found: $requiredTool"
    }
}
foreach ($requiredInput in @($HardwareDefinition, $Bitstream, $BootloaderElf))
{
    if (!(Test-Path -LiteralPath $requiredInput))
    {
        throw "Boot image input not found: $requiredInput"
    }
}

if (!$buildGeneratedFsbl -and !(Test-Path -LiteralPath $FsblElf))
{
    throw "Explicit FSBL ELF not found: $FsblElf"
}

if ($buildGeneratedFsbl)
{
    $fsblWorkspace = Join-Path $buildRoot "fsbl_workspace"
    if (Test-Path -LiteralPath $fsblWorkspace)
    {
        $resolvedWorkspace = (Resolve-Path -LiteralPath $fsblWorkspace).Path
        $resolvedBuildRoot = (Resolve-Path -LiteralPath $buildRoot).Path
        if (!$resolvedWorkspace.StartsWith($resolvedBuildRoot + "\", [System.StringComparison]::OrdinalIgnoreCase))
        {
            throw "Refusing to replace FSBL workspace outside the bootloader build directory: $resolvedWorkspace"
        }
        Remove-Item -LiteralPath $resolvedWorkspace -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $fsblWorkspace | Out-Null
    & $xsct (Join-Path $projectRoot "build_fsbl.tcl") $HardwareDefinition $fsblWorkspace
    if (($LASTEXITCODE -ne 0) -or !(Test-Path -LiteralPath $FsblElf))
    {
        throw "Zynq FSBL build failed or did not produce the expected ELF: $FsblElf"
    }
}

$normalizeForBif = {
    param([string]$Path)
    return (Resolve-Path -LiteralPath $Path).Path.Replace("\", "/")
}
$bifPath = Join-Path $buildRoot "zynq7020_bootloader.bif"
$bootImage = Join-Path $buildRoot "BOOT.bin"
$bifText = @"
the_ROM_image:
{
    [bootloader] $(& $normalizeForBif $FsblElf)
    $(& $normalizeForBif $Bitstream)
    $(& $normalizeForBif $BootloaderElf)
}
"@
[System.IO.File]::WriteAllText($bifPath, $bifText, [System.Text.UTF8Encoding]::new($false))

& $bootgen -image $bifPath -arch zynq -o $bootImage -w on
if ($LASTEXITCODE -ne 0)
{
    throw "Zynq boot image generation failed with exit code $LASTEXITCODE"
}

$bootImageSize = (Get-Item -LiteralPath $bootImage).Length
if ($bootImageSize -gt $bootPartitionSize)
{
    throw ("BOOT.bin size {0} exceeds protected boot partition size {1}" -f $bootImageSize, $bootPartitionSize)
}

$alignedSize = [Math]::Ceiling($bootImageSize / 65536.0) * 65536
Write-Host ("BOOT_IMAGE_RESULT status=PASS image={0} size={1} erase_aligned_size={2} partition_size={3}" -f `
    $bootImage, $bootImageSize, $alignedSize, $bootPartitionSize)
