# Compile the current NAND request-entry functions with a hardware-start stub.
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$driver = [IO.File]::ReadAllText((Join-Path $repoRoot 'platform/gd32e507/bsp/src/bsp_nand_flash.c'), [Text.Encoding]::UTF8)
$start = $driver.IndexOf('static BSP_FLASH_RESULT_E begin(')
$end = $driver.IndexOf('const bsp_flash_chip_t bsp_nand_chip', $start)
if ($start -lt 0 -or $end -le $start) { throw 'NAND entry-function boundaries not found' }
$entries = $driver.Substring($start, $end - $start)
$fixture = @'
#include "bsp_flash.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define NAND_PAGE 2048u
#define NAND_BLOCK (64u * NAND_PAGE)
#define NAND_CAPACITY (1024u * NAND_BLOCK)
enum { NAND_IDLE, NAND_MARK0, NAND_MARK1, NAND_READ, NAND_PROGRAM, NAND_ERASE };
static unsigned int phase;
static bsp_flash_health_t health;
static uint8_t rejected[1024], erased[1024], next_page[1024];
static uint32_t current_address, remaining;
static uint8_t operation;
static uint8_t *p_destination;
static const uint8_t *p_source;
static unsigned int starts, checks, failures;
static void read_start(uint32_t row, uint32_t column)
{
    (void)row; (void)column; starts++;
}
static void check(const char *p_name, int passed)
{
    checks++;
    if (passed == 0) { failures++; printf("FAIL %s\n", p_name); }
}
'@
$cases = @'
int main(void)
{
    uint8_t original[NAND_PAGE] = {0};
    uint8_t replacement[NAND_PAGE] = {0};
    for (unsigned int state = NAND_MARK0; state <= NAND_ERASE; state++)
    {
        phase = state; health.error = 0; starts = 0;
        p_destination = original; p_source = original;
        current_address = 2048u; remaining = 512u; operation = 1u;
        check("busy read rejected", read_data(0u, 256u, replacement) == BSP_FLASH_INVALID_ARGUMENT);
        check("busy program rejected", program(0u, NAND_PAGE, replacement) == BSP_FLASH_INVALID_ARGUMENT);
        check("busy erase rejected", erase(0u, NAND_BLOCK) == BSP_FLASH_INVALID_ARGUMENT);
        check("active destination preserved", p_destination == original);
        check("active source preserved", p_source == original);
        check("active state preserved", phase == state && current_address == 2048u &&
              remaining == 512u && operation == 1u && health.error == 0 && starts == 0u);
    }
    phase = NAND_IDLE;
    check("invalid read rejected", read_data(NAND_CAPACITY, 1u, replacement) == BSP_FLASH_INVALID_ARGUMENT);
    check("invalid read preserves pointer", p_destination == original);
    check("invalid program rejected", program(NAND_CAPACITY, NAND_PAGE, replacement) == BSP_FLASH_INVALID_ARGUMENT);
    check("invalid program preserves pointer", p_source == original);
    check("null read rejected", read_data(0u, 1u, NULL) == BSP_FLASH_INVALID_ARGUMENT);
    check("null program rejected", program(0u, NAND_PAGE, NULL) == BSP_FLASH_INVALID_ARGUMENT);
    check("read accepted", read_data(0u, 256u, replacement) == BSP_FLASH_SUCCESS);
    check("accepted read owns new buffer", p_destination == replacement && phase == NAND_MARK0 && starts == 1u);
    phase = NAND_IDLE; erased[0] = 1u;
    check("program accepted", program(0u, NAND_PAGE, replacement) == BSP_FLASH_SUCCESS);
    check("accepted program owns new buffer", p_source == replacement && phase == NAND_MARK0 && starts == 2u);
    printf("SUMMARY NAND entry checks=%u failures=%u\n", checks, failures);
    return failures == 0u ? 0 : 1;
}
'@
$testExe = Join-Path $PSScriptRoot 'test_nand_request.exe'
$fixture + "`n" + $entries + "`n" + $cases | & 'C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe' -x c - -std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef "-I$repoRoot/platform/gd32e507/bsp/inc" -o $testExe
if ($LASTEXITCODE -ne 0) { throw 'NAND entry regression compilation failed' }
& $testExe
if ($LASTEXITCODE -ne 0) { throw 'NAND entry regression failed' }
