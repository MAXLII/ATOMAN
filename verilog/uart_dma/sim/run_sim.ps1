param(
    [string]$IcarusRoot = "C:\iverilog"
)

$ErrorActionPreference = "Stop"
$simRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$verilogRoot = Split-Path -Parent $simRoot
$repoRoot = Split-Path -Parent (Split-Path -Parent $verilogRoot)
$buildRoot = Join-Path $repoRoot "build\uart_dma_sim"
$iverilog = Join-Path $IcarusRoot "bin\iverilog.exe"
$vvp = Join-Path $IcarusRoot "bin\vvp.exe"
$coreSimulation = Join-Path $buildRoot "tb_uart_serial_core.vvp"
$dmaSimulation = Join-Path $buildRoot "tb_axi_uart_dma.vvp"

foreach ($requiredTool in @($iverilog, $vvp))
{
    if (!(Test-Path -LiteralPath $requiredTool))
    {
        throw "Required Icarus Verilog tool not found: $requiredTool"
    }
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

& $iverilog -g2012 -Wall -s tb_uart_serial_core `
    -o $coreSimulation `
    (Join-Path $verilogRoot "rtl\uart_serial_core.v") `
    (Join-Path $simRoot "tb_uart_serial_core.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "UART core simulation compile failed with exit code $LASTEXITCODE"
}

& $vvp $coreSimulation
if ($LASTEXITCODE -ne 0)
{
    throw "UART core simulation failed with exit code $LASTEXITCODE"
}

& $iverilog -g2012 -Wall -s tb_axi_uart_dma `
    -o $dmaSimulation `
    (Join-Path $verilogRoot "rtl\uart_sync_fifo.v") `
    (Join-Path $verilogRoot "rtl\uart_serial_core.v") `
    (Join-Path $verilogRoot "rtl\axi_uart_dma.v") `
    (Join-Path $simRoot "tb_axi_uart_dma.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "UART DMA simulation compile failed with exit code $LASTEXITCODE"
}

& $vvp $dmaSimulation
if ($LASTEXITCODE -ne 0)
{
    throw "UART DMA simulation failed with exit code $LASTEXITCODE"
}

Write-Output "UART_DMA_RTL_SIMULATION result=PASS"
