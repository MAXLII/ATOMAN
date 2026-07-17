param(
    [string]$XilinxVivado = "C:\Xilinx\Vivado\2018.3"
)

$ErrorActionPreference = "Stop"
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vivado = Join-Path $XilinxVivado "bin\vivado.bat"

if (!(Test-Path -LiteralPath $vivado))
{
    throw "Vivado 2018.3 not found: $vivado"
}

Push-Location $scriptRoot
try
{
    & $vivado -mode batch -source (Join-Path $scriptRoot "build_pl.tcl")
    if ($LASTEXITCODE -ne 0)
    {
        throw "Zynq-7020 PL build failed with exit code $LASTEXITCODE"
    }
}
finally
{
    Pop-Location
}
