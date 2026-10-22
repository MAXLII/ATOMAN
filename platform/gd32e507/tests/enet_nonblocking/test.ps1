$ErrorActionPreference = 'Stop'
$platform = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$lwip = Join-Path $platform 'third_party/lwip-2.2.1'
$output = Join-Path $platform 'build_realtime/test_enet_nonblocking.exe'
$sources = @("$lwip/port/GD32E50x/Basic/ethernetif.c", "$lwip/src/netif/ethernet.c")
$sources += (Get-ChildItem "$lwip/src/core/*.c", "$lwip/src/core/ipv4/*.c").FullName
$includes = @('-I', $PSScriptRoot, '-I', "$platform/enet/inc", '-I', "$lwip/src/include", '-I', "$lwip/port/GD32E50x", '-I', "$lwip/port/GD32E50x/Basic")
$object = "$output.o"
& gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef @includes -c "$PSScriptRoot/test.c" -o $object
if ($LASTEXITCODE) { throw 'Strict host test compilation failed' }
# The existing vendor port uses ARM printf formats and empty assertion bodies.
& gcc -std=c11 -Wall -Wextra -Werror -Wno-format -Wno-empty-body -Wno-unused-parameter @includes @sources $object -o $output
if ($LASTEXITCODE) { throw 'Production Ethernet/LwIP host build failed' }
$log = "$output.log"
$process = Start-Process -FilePath $output -WindowStyle Hidden -RedirectStandardOutput $log -PassThru
if (!$process.WaitForExit(5000)) {
    $process.Kill()
    throw 'Ethernet output blocked for over five seconds'
}
Get-Content $log -Encoding UTF8
if ($process.ExitCode) { throw 'Nonblocking Ethernet regression failed' }
