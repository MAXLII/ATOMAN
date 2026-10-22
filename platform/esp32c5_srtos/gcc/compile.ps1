param(
    [string]$IdfPath = $env:IDF_PATH,
    [string]$IdfToolsPath = $env:IDF_TOOLS_PATH,
    [string]$PythonPath,
    [string[]]$IdfArguments = @('build'),
    [string]$ProjectPath,
    [string]$BuildPath,
    [string]$WifiConfigPath,
    [string]$Port,
    [switch]$Flash
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ($ProjectPath) { $projectRoot = (Resolve-Path -LiteralPath $ProjectPath).Path }
if (-not $BuildPath) { $BuildPath = Join-Path $projectRoot 'build' }
$BuildPath = [IO.Path]::GetFullPath($BuildPath)
if ($Flash) {
    if (-not $Port) { throw 'Flash requires -Port COMx' }
    $IdfArguments = @('-p', $Port, 'flash')
}
if ($WifiConfigPath) {
    $station = Get-Content -LiteralPath $WifiConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $ssidBytes = [Text.Encoding]::UTF8.GetByteCount([string]$station.ssid)
    $passwordBytes = [Text.Encoding]::UTF8.GetByteCount([string]$station.password)
    if ($ssidBytes -lt 1 -or $ssidBytes -gt 32) { throw 'SSID must contain 1..32 UTF-8 bytes' }
    if ($passwordBytes -ne 0 -and ($passwordBytes -lt 8 -or $passwordBytes -gt 63)) { throw 'PSK must be empty or 8..63 bytes' }
    $channel = [int]$station.channel
    if ($channel -lt 0 -or $channel -gt 165) { throw 'Invalid channel' }
    $privateDirectory = Join-Path $BuildPath 'private'
    New-Item -ItemType Directory -Path $privateDirectory -Force | Out-Null
    $header = '#define ESP32C5_WIFI_SSID ' + (ConvertTo-Json -InputObject ([string]$station.ssid) -Compress) + "`n"
    $header += '#define ESP32C5_WIFI_PASSWORD ' + (ConvertTo-Json -InputObject ([string]$station.password) -Compress) + "`n"
    $header += '#define ESP32C5_WIFI_CHANNEL ' + $channel.ToString() + "u`n"
    [IO.File]::WriteAllText((Join-Path $privateDirectory 'wifi_config.h'), $header, [Text.UTF8Encoding]::new($false))
}
if (-not $IdfPath) { $IdfPath = 'D:\Espressif\v6.1\esp-idf' }
if (-not $IdfToolsPath) { $IdfToolsPath = 'C:\Espressif\tools' }
if (-not $PythonPath) { $PythonPath = Join-Path $IdfToolsPath 'python\v6.1\venv\Scripts\python.exe' }
foreach ($required in @((Join-Path $IdfPath 'tools\idf.py'), $PythonPath)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing tool: $required. Supply -IdfPath/-IdfToolsPath/-PythonPath." }
}
# Use installed tools without invoking EIM/profile scripts that change global selection.
$toolPatterns = @('riscv32-esp-elf\*\riscv32-esp-elf\bin', 'cmake\*\bin', 'ninja\*')
$toolBins = foreach ($pattern in $toolPatterns) {
    $resolved = @(Get-Item -Path (Join-Path $IdfToolsPath $pattern) | Sort-Object FullName)
    if ($resolved.Count -eq 0) { throw "Missing installed tool directory: $pattern" }
    $resolved[-1].FullName
}
$env:IDF_PATH = $IdfPath
$env:IDF_TOOLS_PATH = $IdfToolsPath
$env:IDF_PYTHON_ENV_PATH = Split-Path (Split-Path $PythonPath)
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PYTHONUTF8 = '1'
$versionText = Get-Content -Raw -Encoding UTF8 (Join-Path $IdfPath 'tools\cmake\version.cmake')
$versionParts = foreach ($part in @('MAJOR', 'MINOR', 'PATCH')) {
    $match = [regex]::Match($versionText, "set\(IDF_VERSION_$part ([0-9]+)\)")
    if (-not $match.Success) { throw "Cannot read SDK version $part" }
    $match.Groups[1].Value
}
$env:ESP_IDF_VERSION = $versionParts -join '.'
$romDirectories = @(Get-ChildItem -LiteralPath (Join-Path $IdfToolsPath 'esp-rom-elfs') -Directory | Sort-Object Name)
if ($romDirectories.Count -eq 0) { throw 'Missing ESP ROM ELF package' }
$env:ESP_ROM_ELF_DIR = $romDirectories[-1].FullName
$env:IDF_CCACHE_ENABLE = '0'
$env:PATH = (@((Split-Path $PythonPath)) + $toolBins + @($env:PATH)) -join ';'
Push-Location $projectRoot
try {
    & $PythonPath (Join-Path $IdfPath 'tools\idf.py') -B $BuildPath @IdfArguments
    if ($LASTEXITCODE -ne 0) { throw "idf.py failed with exit code $LASTEXITCODE" }
}
finally { Pop-Location }
