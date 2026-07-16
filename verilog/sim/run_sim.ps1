param(
    [string]$IcarusRoot = "C:\iverilog"
)

$ErrorActionPreference = "Stop"
$simRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$verilogRoot = Split-Path -Parent $simRoot
$repoRoot = Split-Path -Parent $verilogRoot
$buildRoot = Join-Path $repoRoot "build\verilog_sim"
$iverilog = Join-Path $IcarusRoot "bin\iverilog.exe"
$vvp = Join-Path $IcarusRoot "bin\vvp.exe"
$coreSimulation = Join-Path $buildRoot "tb_iir_3p3z_core.vvp"
$axiSimulation = Join-Path $buildRoot "tb_axi_iir_3p3z.vvp"

if (!(Test-Path -LiteralPath $iverilog))
{
    throw "Icarus Verilog compiler not found: $iverilog"
}

if (!(Test-Path -LiteralPath $vvp))
{
    throw "Icarus Verilog runtime not found: $vvp"
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

& $iverilog -g2012 -Wall -s tb_iir_3p3z_core `
    -o $coreSimulation `
    (Join-Path $verilogRoot "src\iir_3p3z_core.v") `
    (Join-Path $simRoot "tb_iir_3p3z_core.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "Standalone 3P3Z IIR simulation compile failed with exit code $LASTEXITCODE"
}

& $iverilog -g2012 -Wall -s tb_axi_iir_3p3z `
    -o $axiSimulation `
    (Join-Path $verilogRoot "src\iir_3p3z_core.v") `
    (Join-Path $verilogRoot "src\axi_iir_3p3z.v") `
    (Join-Path $simRoot "tb_axi_iir_3p3z.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "3P3Z IIR simulation compile failed with exit code $LASTEXITCODE"
}

Push-Location $simRoot
try
{
    & $vvp $coreSimulation
    if ($LASTEXITCODE -ne 0)
    {
        throw "Standalone 3P3Z IIR simulation failed with exit code $LASTEXITCODE"
    }

    & $vvp $axiSimulation
    if ($LASTEXITCODE -ne 0)
    {
        throw "3P3Z IIR simulation failed with exit code $LASTEXITCODE"
    }
}
finally
{
    Pop-Location
}
