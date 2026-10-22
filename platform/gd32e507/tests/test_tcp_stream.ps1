$ErrorActionPreference='Stop'
$platform=Split-Path $PSScriptRoot -Parent
$output=Join-Path $platform 'build_realtime/test_tcp_stream.exe'
& gcc -std=c11 -Wall -Wextra -Werror -I "$PSScriptRoot/stubs" -I "$platform/bsp/inc" "$PSScriptRoot/test_tcp_stream.c" "$platform/bsp/src/bsp_tcp_stream.c" -o $output
if($LASTEXITCODE){throw 'BSP host test build failed'}
& $output
if($LASTEXITCODE){throw 'BSP host test failed'}
