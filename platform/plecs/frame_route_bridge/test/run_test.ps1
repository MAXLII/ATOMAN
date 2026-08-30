$ErrorActionPreference = "Stop"

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = (Resolve-Path (Join-Path $testDirectory "..\..\..\..")).Path
$projectDirectory = Join-Path $repositoryRoot "platform\plecs\frame_route_bridge"
$compileScript = Join-Path $projectDirectory "compile.bat"
$testScript = Join-Path $testDirectory "test_plecs_route_bridge.py"

Push-Location $projectDirectory
try {
    & $compileScript
    if ($LASTEXITCODE -ne 0) {
        throw "PLECS route-bridge DLL build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

python $testScript
if ($LASTEXITCODE -ne 0) {
    throw "PLECS route-bridge smoke test failed with exit code $LASTEXITCODE"
}
