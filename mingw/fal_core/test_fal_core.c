// SPDX-License-Identifier: MIT
/**
 * @file    test_fal_core.c
 * @brief   Isolated host tests for the real FAL core.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Verify physical address, page, read, and erase splitting
 *          - Verify configuration, permission, busy, and stop behavior
 *          - Produce deterministic console and file logs for every assertion
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Test process is single-threaded
 *          - Hardware access is replaced by fake_flash
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "fake_fal_cfg.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static FILE *p_log_file = NULL; /* Detailed test log stream. */
static uint32_t check_count = 0u; /* Total assertions evaluated. */
static uint32_t fail_count = 0u;  /* Assertions that failed. */

static void log_line(const char *p_format, ...)
{
    va_list args_console; /* Console formatting arguments. */
    va_list args_file;    /* File formatting arguments. */

    va_start(args_console, p_format);
    va_copy(args_file, args_console);
    (void)vprintf(p_format, args_console);
    va_end(args_console);
    if (p_log_file != NULL)
    {
        (void)vfprintf(p_log_file, p_format, args_file);
        (void)fflush(p_log_file);
    }
    va_end(args_file);
}

static void check_i32(const char *p_name, int32_t expected, int32_t actual)
{
    uint8_t passed = (expected == actual) ? 1u : 0u; /* Current assertion result. */

    check_count++;
    if (passed == 0u)
    {
        fail_count++;
    }
    log_line("CHECK %03lu %-38s expected=%ld actual=%ld %s\n",
             (unsigned long)check_count,
             p_name,
             (long)expected,
             (long)actual,
             (passed == 1u) ? "PASS" : "FAIL");
}

static void process_until_idle(fal_t *p_fal, uint32_t max_steps)
{
    uint32_t step = 0u; /* State-machine steps executed for this operation. */

    for (step = 0u; (step < max_steps) && (fal_is_busy(p_fal) == 1u); step++)
    {
        fal_process(p_fal);
    }
    log_line("OUTPUT process steps=%lu state=%d result=%d\n",
             (unsigned long)step,
             (int)fal_state_get(p_fal),
             (int)fal_result_get(p_fal));
}

static void case_write_split_and_read(void)
{
    fake_fal_fixture_t fixture = {0}; /* Independent fake platform configuration. */
    fal_t fal = {0};                  /* Real FAL state-machine instance. */
    uint8_t write_data[30] = {0};     /* Program source crossing 3 physical pages. */
    uint8_t read_data[30] = {0};      /* Read destination split by max_read_size. */
    uint32_t index = 0u;              /* Test data byte index. */

    log_line("\nCASE write_split_and_read\n");
    fake_fal_fixture_reset(&fixture);
    for (index = 0u; index < sizeof(write_data); index++)
    {
        write_data[index] = (uint8_t)(index + 1u);
    }
    check_i32("fal_init", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    check_i32("submit write", FAL_RESULT_IN_PROGRESS,
              fal_write(&fal, FAKE_FAL_ZONE_IAP, 7u, sizeof(write_data), write_data));
    process_until_idle(&fal, 64u);
    check_i32("write result", FAL_RESULT_SUCCESS, fal_result_get(&fal));
    check_i32("program+sync call count", 4, (int32_t)fixture.first_flash.call_count);
    check_i32("first program address", 519, (int32_t)fixture.first_flash.calls[0].address);
    check_i32("first program length", 9, (int32_t)fixture.first_flash.calls[0].length);
    check_i32("second program length", 16, (int32_t)fixture.first_flash.calls[1].length);
    check_i32("third program length", 5, (int32_t)fixture.first_flash.calls[2].length);

    fixture.first_flash.call_count = 0u;
    check_i32("submit read", FAL_RESULT_IN_PROGRESS,
              fal_read(&fal, FAKE_FAL_ZONE_IAP, 7u, sizeof(read_data), read_data));
    process_until_idle(&fal, 64u);
    check_i32("read result", FAL_RESULT_SUCCESS, fal_result_get(&fal));
    check_i32("read data", 0, memcmp(write_data, read_data, sizeof(write_data)));
    check_i32("read+sync call count", 3, (int32_t)fixture.first_flash.call_count);
    check_i32("first read length", 24, (int32_t)fixture.first_flash.calls[0].length);
    check_i32("second read length", 6, (int32_t)fixture.first_flash.calls[1].length);
}

static void case_erase_and_permission(void)
{
    fake_fal_fixture_t fixture = {0}; /* Independent fake platform configuration. */
    fal_t fal = {0};                  /* Real FAL state-machine instance. */
    uint8_t value = 0x00u;            /* Byte used to test protected-zone access. */

    log_line("\nCASE erase_and_permission\n");
    fake_fal_fixture_reset(&fixture);
    check_i32("fal_init", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    check_i32("boot write denied", FAL_RESULT_PERMISSION_DENIED,
              fal_write(&fal, FAKE_FAL_ZONE_BOOT, 0u, 1u, &value));
    check_i32("boot erase denied", FAL_RESULT_PERMISSION_DENIED,
              fal_erase(&fal, FAKE_FAL_ZONE_BOOT, 0u, 1u));
    check_i32("submit unaligned erase", FAL_RESULT_IN_PROGRESS,
              fal_erase(&fal, FAKE_FAL_ZONE_IAP, 10u, 100u));
    process_until_idle(&fal, 64u);
    check_i32("erase result", FAL_RESULT_SUCCESS, fal_result_get(&fal));
    check_i32("erase+sync call count", 3, (int32_t)fixture.first_flash.call_count);
    check_i32("first erase address", 512, (int32_t)fixture.first_flash.calls[0].address);
    check_i32("second erase address", 576, (int32_t)fixture.first_flash.calls[1].address);
}

static void case_busy_error_and_stop(void)
{
    fake_fal_fixture_t fixture = {0}; /* Independent fake platform configuration. */
    fal_t fal = {0};                  /* Real FAL state-machine instance. */
    uint8_t data[8] = {0};            /* Small operation buffer. */

    log_line("\nCASE busy_error_and_stop\n");
    fake_fal_fixture_reset(&fixture);
    fixture.first_flash.busy_polls_per_operation = 10u;
    check_i32("fal_init", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    check_i32("submit read", FAL_RESULT_IN_PROGRESS,
              fal_read(&fal, FAKE_FAL_ZONE_IAP, 0u, sizeof(data), data));
    check_i32("reject concurrent erase", FAL_RESULT_BUSY,
              fal_erase(&fal, FAKE_FAL_ZONE_STAGING, 0u, 64u));
    check_i32("request deferred stop", FAL_RESULT_SUCCESS, fal_stop_request(&fal));
    process_until_idle(&fal, 64u);
    check_i32("stopped after completion", 1, fal_is_stopped(&fal));
    check_i32("new request rejected", FAL_RESULT_STOPPED,
              fal_read(&fal, FAKE_FAL_ZONE_IAP, 0u, sizeof(data), data));

    fake_fal_fixture_reset(&fixture);
    check_i32("reinitialize", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    fixture.first_flash.next_result = FAL_RESULT_DRIVER_ERROR;
    check_i32("submit failing erase", FAL_RESULT_IN_PROGRESS,
              fal_erase(&fal, FAKE_FAL_ZONE_IAP, 0u, 64u));
    process_until_idle(&fal, 8u);
    check_i32("driver error result", FAL_RESULT_DRIVER_ERROR, fal_result_get(&fal));

    fake_fal_fixture_reset(&fixture);
    check_i32("reinitialize for read failure", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    fixture.first_flash.next_result = FAL_RESULT_DRIVER_ERROR;
    check_i32("submit failing read", FAL_RESULT_IN_PROGRESS,
              fal_read(&fal, FAKE_FAL_ZONE_IAP, 0u, sizeof(data), data));
    process_until_idle(&fal, 8u);
    check_i32("read driver error result", FAL_RESULT_DRIVER_ERROR, fal_result_get(&fal));

    fake_fal_fixture_reset(&fixture);
    check_i32("reinitialize for write failure", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    fixture.first_flash.next_result = FAL_RESULT_DRIVER_ERROR;
    check_i32("submit failing write", FAL_RESULT_IN_PROGRESS,
              fal_write(&fal, FAKE_FAL_ZONE_IAP, 0u, sizeof(data), data));
    process_until_idle(&fal, 8u);
    check_i32("write driver error result", FAL_RESULT_DRIVER_ERROR, fal_result_get(&fal));
}

static void case_invalid_cfg_and_second_device(void)
{
    fake_fal_fixture_t fixture = {0}; /* Mutable configuration used for validation tests. */
    fal_t fal = {0};                  /* Real FAL state-machine instance. */
    uint8_t data[4] = {1u, 2u, 3u, 4u}; /* Program data targeting the second device. */

    log_line("\nCASE invalid_cfg_and_second_device\n");
    fake_fal_fixture_reset(&fixture);
    fixture.zones[2].device_offset = fixture.zones[1].device_offset;
    check_i32("reject overlapping zones", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.zones[0].device_id = 99u;
    check_i32("reject unknown device", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.devices[1].device_id = fixture.devices[0].device_id;
    check_i32("reject duplicate device id", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.zones[1].zone_id = fixture.zones[0].zone_id;
    check_i32("reject duplicate zone id", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.devices[0].program_page_size = 0u;
    check_i32("reject invalid geometry", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.devices[0].ops.p_program = NULL;
    check_i32("reject missing driver function", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    fixture.zones[1].device_offset++;
    check_i32("reject unaligned zone", FAL_RESULT_CONFIG_ERROR, fal_init(&fal, &fixture.cfg));

    fake_fal_fixture_reset(&fixture);
    check_i32("fal_init", FAL_RESULT_SUCCESS, fal_init(&fal, &fixture.cfg));
    check_i32("submit second-device write", FAL_RESULT_IN_PROGRESS,
              fal_write(&fal, FAKE_FAL_ZONE_SECOND, 5u, sizeof(data), data));
    process_until_idle(&fal, 32u);
    check_i32("second device used", 0, (int32_t)fixture.first_flash.call_count);
    check_i32("second physical address", 261, (int32_t)fixture.second_flash.calls[0].address);
    check_i32("out of range", FAL_RESULT_OUT_OF_RANGE,
              fal_read(&fal, FAKE_FAL_ZONE_SECOND, 1023u, 2u, data));
    check_i32("overflow-sized offset", FAL_RESULT_OUT_OF_RANGE,
              fal_read(&fal, FAKE_FAL_ZONE_SECOND, UINT32_MAX, 1u, data));
    check_i32("invalid zone", FAL_RESULT_INVALID_ARGUMENT,
              fal_read(&fal, 0xFFFFu, 0u, 1u, data));
    check_i32("null read buffer", FAL_RESULT_INVALID_ARGUMENT,
              fal_read(&fal, FAKE_FAL_ZONE_SECOND, 0u, 1u, NULL));
    check_i32("zero length succeeds", FAL_RESULT_SUCCESS,
              fal_read(&fal, FAKE_FAL_ZONE_SECOND, 1024u, 0u, NULL));
}

static void case_multiple_instances(void)
{
    fake_fal_fixture_t first_fixture = {0};  /* Storage owned by the first FAL instance. */
    fake_fal_fixture_t second_fixture = {0}; /* Storage owned by the second FAL instance. */
    fal_t first_fal = {0};                   /* First independently advanced state machine. */
    fal_t second_fal = {0};                  /* Second independently advanced state machine. */
    uint8_t first_data[4] = {1u, 2u, 3u, 4u}; /* First device payload. */
    uint8_t second_data[4] = {5u, 6u, 7u, 8u}; /* Second device payload. */
    uint32_t step = 0u;                        /* Interleaved process iteration. */

    log_line("\nCASE multiple_instances\n");
    fake_fal_fixture_reset(&first_fixture);
    fake_fal_fixture_reset(&second_fixture);
    first_fixture.first_flash.busy_polls_per_operation = 2u;
    second_fixture.first_flash.busy_polls_per_operation = 4u;
    check_i32("first instance init", FAL_RESULT_SUCCESS, fal_init(&first_fal, &first_fixture.cfg));
    check_i32("second instance init", FAL_RESULT_SUCCESS, fal_init(&second_fal, &second_fixture.cfg));
    check_i32("first instance submit", FAL_RESULT_IN_PROGRESS,
              fal_write(&first_fal, FAKE_FAL_ZONE_IAP, 0u, sizeof(first_data), first_data));
    check_i32("second instance submit", FAL_RESULT_IN_PROGRESS,
              fal_write(&second_fal, FAKE_FAL_ZONE_IAP, 8u, sizeof(second_data), second_data));
    for (step = 0u; step < 32u; step++)
    {
        fal_process(&first_fal);
        fal_process(&second_fal);
    }
    check_i32("first instance result", FAL_RESULT_SUCCESS, fal_result_get(&first_fal));
    check_i32("second instance result", FAL_RESULT_SUCCESS, fal_result_get(&second_fal));
    check_i32("first instance isolated data", 0,
              memcmp(&first_fixture.first_flash.data[512], first_data, sizeof(first_data)));
    check_i32("second instance isolated data", 0,
              memcmp(&second_fixture.first_flash.data[520], second_data, sizeof(second_data)));
}

int main(void)
{
    p_log_file = fopen("test_fal_core.log", "w");
    if (p_log_file == NULL)
    {
        (void)fprintf(stderr, "failed to open test_fal_core.log\n");
        return 2;
    }

    case_write_split_and_read();
    case_erase_and_permission();
    case_busy_error_and_stop();
    case_invalid_cfg_and_second_device();
    case_multiple_instances();

    log_line("\nSUMMARY checks=%lu passed=%lu failed=%lu\n",
             (unsigned long)check_count,
             (unsigned long)(check_count - fail_count),
             (unsigned long)fail_count);
    (void)fclose(p_log_file);
    p_log_file = NULL;
    return (fail_count == 0u) ? 0 : 1;
}
