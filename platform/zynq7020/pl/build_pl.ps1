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
    $buildTree = Join-Path $scriptRoot "build"
    $vivadoProcess = Start-Process `
        -FilePath $vivado `
        -ArgumentList @("-mode", "batch", "-source", (Join-Path $scriptRoot "build_pl.tcl")) `
        -WorkingDirectory $scriptRoot `
        -WindowStyle Hidden `
        -PassThru

    while (!$vivadoProcess.WaitForExit(2000))
    {
        if (Test-Path -LiteralPath $buildTree)
        {
            & attrib.exe -R (Join-Path $buildTree "*") /S /D | Out-Null
        }
    }

    $vivadoProcess.WaitForExit()
    if ($vivadoProcess.ExitCode -ne 0)
    {
        throw "Zynq-7020 PL build failed with exit code $($vivadoProcess.ExitCode)"
    }
}
finally
{
    Pop-Location
}
