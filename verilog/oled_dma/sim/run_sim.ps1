param(
    [string]$IcarusRoot = "C:\iverilog"
)

$ErrorActionPreference = "Stop"
$simRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$verilogRoot = Split-Path -Parent $simRoot
$repoRoot = Split-Path -Parent (Split-Path -Parent $verilogRoot)
$buildRoot = Join-Path $repoRoot "build\oled_dma_sim"
$iverilog = Join-Path $IcarusRoot "bin\iverilog.exe"
$vvp = Join-Path $IcarusRoot "bin\vvp.exe"

foreach ($requiredTool in @($iverilog, $vvp))
{
    if (!(Test-Path -LiteralPath $requiredTool))
    {
        throw "Required Icarus Verilog tool not found: $requiredTool"
    }
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

$rtlFiles = @(
    (Join-Path $verilogRoot "src\oled_serial_phy.v"),
    (Join-Path $verilogRoot "src\oled_frame_ram.v"),
    (Join-Path $verilogRoot "src\oled_frame_dma.v"),
    (Join-Path $verilogRoot "src\ssd1306_protocol.v"),
    (Join-Path $verilogRoot "src\axi_oled_dma.v")
)

$protocolSimulation = Join-Path $buildRoot "tb_ssd1306_protocol.vvp"
& $iverilog -g2012 -Wall -s tb_ssd1306_protocol `
    -o $protocolSimulation `
    (Join-Path $verilogRoot "src\ssd1306_protocol.v") `
    (Join-Path $simRoot "tb_ssd1306_protocol.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "SSD1306 protocol simulation compile failed: $LASTEXITCODE"
}
& $vvp $protocolSimulation
if ($LASTEXITCODE -ne 0)
{
    throw "SSD1306 protocol simulation failed: $LASTEXITCODE"
}

$dmaSimulation = Join-Path $buildRoot "tb_axi_oled_dma.vvp"
& $iverilog -g2012 -Wall -s tb_axi_oled_dma `
    -o $dmaSimulation `
    $rtlFiles `
    (Join-Path $simRoot "tb_axi_oled_dma.sv")
if ($LASTEXITCODE -ne 0)
{
    throw "OLED DMA simulation compile failed: $LASTEXITCODE"
}
& $vvp $dmaSimulation
if ($LASTEXITCODE -ne 0)
{
    throw "OLED DMA simulation failed: $LASTEXITCODE"
}

Write-Output "OLED_DMA_RTL_SIMULATION result=PASS"
