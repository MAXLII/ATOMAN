[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDirectory ".."))
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$issues = New-Object 'System.Collections.Generic.List[string]'

function Read-RepositoryText
{
    param([string]$RelativePath)

    $path = Join-Path $repositoryRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf))
    {
        $issues.Add(("Missing required file: {0}" -f $RelativePath))
        return ''
    }

    try
    {
        return [System.IO.File]::ReadAllText($path, $strictUtf8)
    }
    catch
    {
        $issues.Add(("File is not strict UTF-8: {0}" -f $RelativePath))
        return ''
    }
}

function Assert-Pattern
{
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )

    if (-not [regex]::IsMatch($Text, $Pattern,
                              [System.Text.RegularExpressions.RegexOptions]::Multiline))
    {
        $issues.Add($Description)
    }
}

$rtlPath = 'verilog/iir/rtl/axi_iir_3p3z.v'
$corePath = 'verilog/iir/rtl/iir_3p3z_core.v'
$packagePath = 'platform/zynq7020/pl/package_axi_iir_ip.tcl'
$buildPath = 'platform/zynq7020/pl/build_pl.tcl'
$bspPath = 'platform/zynq7020/ps/bsp/bsp_iir.h'
$designPath = 'verilog/iir/doc/design/axi_iir_3p3z_design.md'
$usagePath = 'verilog/iir/doc/application/axi_iir_3p3z_usage.md'
$platformPath = 'docs/design/platform/zynq7020/zynq7020_platform.md'

$rtl = Read-RepositoryText $rtlPath
$core = Read-RepositoryText $corePath
$package = Read-RepositoryText $packagePath
$build = Read-RepositoryText $buildPath
$bsp = Read-RepositoryText $bspPath
$design = Read-RepositoryText $designPath
$usage = Read-RepositoryText $usagePath
$platform = Read-RepositoryText $platformPath

$registers = @(
    [pscustomobject]@{ Name = 'CONTROL';      Offset = '00'; Index = '00'; Access = 'write-only' },
    [pscustomobject]@{ Name = 'STATUS';       Offset = '04'; Index = '01'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'INPUT';        Offset = '08'; Index = '02'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'OUTPUT';       Offset = '0C'; Index = '03'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'B0';           Offset = '10'; Index = '04'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'B1';           Offset = '14'; Index = '05'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'B2';           Offset = '18'; Index = '06'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'B3';           Offset = '1C'; Index = '07'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'A1';           Offset = '20'; Index = '08'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'A2';           Offset = '24'; Index = '09'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'A3';           Offset = '28'; Index = '0A'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'SAMPLE_COUNT'; Offset = '2C'; Index = '0B'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'VERSION';      Offset = '30'; Index = '0C'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'FORMAT';       Offset = '34'; Index = '0D'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'X1';           Offset = '38'; Index = '0E'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'X2';           Offset = '3C'; Index = '0F'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'X3';           Offset = '40'; Index = '10'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'Y1';           Offset = '44'; Index = '11'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'Y2';           Offset = '48'; Index = '12'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'Y3';           Offset = '4C'; Index = '13'; Access = 'read-only' },
    [pscustomobject]@{ Name = 'LIMIT_LOWER';  Offset = '50'; Index = '14'; Access = 'read-write' },
    [pscustomobject]@{ Name = 'LIMIT_UPPER';  Offset = '54'; Index = '15'; Access = 'read-write' }
)

foreach ($register in $registers)
{
    $name = [regex]::Escape($register.Name)
    $offset = $register.Offset
    $index = $register.Index
    $access = [regex]::Escape($register.Access)

    Assert-Pattern $bsp ("#define\s+BSP_IIR_{0}_OFFSET\s+0x{1}UL\b" -f $name, $offset) `
        ("BSP offset mismatch for {0}." -f $register.Name)
    Assert-Pattern $rtl ("6'h{0}:\s+register_read\s*=" -f $index) `
        ("RTL read decode mismatch for {0}." -f $register.Name)
    Assert-Pattern $package ('add_axi_iir_register\s+\$address_block\s+{0}\s+0x{1}\s+\\?\s*{2}' -f $name, $offset, $access) `
        ("IP-XACT register mismatch for {0}." -f $register.Name)
    Assert-Pattern $design ('\|\s*`0x{0}`\s*\|\s*`{1}`\s*\|' -f $offset, $name) `
        ("Design register table mismatch for {0}." -f $register.Name)
    Assert-Pattern $usage ("^0x{0}\s+{1}\s+" -f $offset, $name) `
        ("Usage register table mismatch for {0}." -f $register.Name)
}

Assert-Pattern $rtl "RTL_VERSION\s*=\s*32'h0002_0000" 'RTL VERSION value mismatch.'
Assert-Pattern $rtl "FORMAT_VALUE\s*=\s*32'h0000_201E" 'RTL FORMAT value mismatch.'
Assert-Pattern $bsp '#define\s+BSP_IIR_VERSION_VALUE\s+0x00020000UL' 'BSP VERSION value mismatch.'
Assert-Pattern $bsp '#define\s+BSP_IIR_FORMAT_VALUE\s+0x0000201EUL' 'BSP FORMAT value mismatch.'
Assert-Pattern $design '`0x00020000`' 'Design VERSION value mismatch.'
Assert-Pattern $design '`0x0000201E`' 'Design FORMAT value mismatch.'
Assert-Pattern $usage '0x00020000' 'Usage VERSION value mismatch.'
Assert-Pattern $usage '0x0000201E' 'Usage FORMAT value mismatch.'

Assert-Pattern $bsp '#define\s+BSP_IIR_BASE_ADDR\s+0x43C00000UL' 'BSP base address mismatch.'
Assert-Pattern $build '-offset\s+0x43C00000' 'PL address map mismatch.'
Assert-Pattern $design '`0x43C00000`' 'Design base address mismatch.'
Assert-Pattern $usage '0x43C00000' 'Usage base address mismatch.'
Assert-Pattern $platform '`0x43C00000`' 'Platform base address mismatch.'

Assert-Pattern $rtl "coeff_b0_reg\s*<=\s*32'h4000_0000" 'RTL B0 reset value mismatch.'
Assert-Pattern $rtl "limit_lower_reg\s*<=\s*32'h8000_0000" 'RTL lower-limit reset mismatch.'
Assert-Pattern $rtl "limit_upper_reg\s*<=\s*32'h7FFF_FFFF" 'RTL upper-limit reset mismatch.'

$noDspCount = [regex]::Matches($core, '\(\*\s*use_dsp\s*=\s*"no"\s*\*\)').Count
if ($noDspCount -ne 7)
{
    $issues.Add(("Expected 7 use_dsp=no multipliers, found {0}." -f $noDspCount))
}
Assert-Pattern $design 'use_dsp="no"' 'Design document does not describe LUT multiplier binding.'
Assert-Pattern $platform 'use_dsp="no"' 'Platform document does not describe LUT multiplier binding.'
Assert-Pattern $usage 'DSP\s+.*0' 'Usage document does not state the current zero-DSP result.'

Write-Host ("IIR registers : {0}" -f $registers.Count)
Write-Host ("Issues        : {0}" -f $issues.Count)

if ($issues.Count -gt 0)
{
    foreach ($issue in $issues)
    {
        Write-Host $issue -ForegroundColor Red
    }
    exit 1
}

Write-Host 'IIR documentation and implementation are consistent.' -ForegroundColor Green
exit 0
