param(
    [string]$XilinxVivado = "C:\Xilinx\Vivado\2018.3"
)

$ErrorActionPreference = "Stop"
$simRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vivado = Join-Path $XilinxVivado "bin\vivado.bat"

if (!(Test-Path -LiteralPath $vivado))
{
    throw "Vivado 2018.3 not found: $vivado"
}

Push-Location $simRoot
try
{
    & $vivado -mode batch -source (Join-Path $simRoot "run_synth.tcl")
    if ($LASTEXITCODE -ne 0)
    {
        throw "UART DMA OOC synthesis failed with exit code $LASTEXITCODE"
    }
}
finally
{
    Pop-Location
}
