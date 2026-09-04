// SPDX-License-Identifier: MIT
/**
 * @file    fal_board_test.c
 * @brief   Run opt-in bounded FAL stress and rejection checks on real Flash.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Exercise actual Demo, Core and Interface APIs from task context.
 *          - Limit accepted erase/program requests to configured test partitions.
 *          - Stop on the first failure and publish durable-in-RAM evidence.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Not ISR-safe; one task owns test buffers and submits each operation.
 *          - Hardware operations go through Interface/BSP; explicit opt-in build.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * This file is licensed under the MIT License. See LICENSE in the project root.
 */
#include "fal_board_test.h"
#include "demo_storage_business.h"
#include "fal_cfg.h"
#include "flash_port.h"
#include "section.h"
#include "systick.h"
#include <string.h>

#define TEST_NAND_BASE 0x07F40000u /* First NAND test block, never parameters/logs. */
#define TEST_PAGE 2048u           /* One complete NAND program page. */
#define TEST_TIMEOUT 300000u     /* 30 seconds at the 100 us platform tick. */
volatile fal_board_report_t g_fal_board_report = {0}; /* SWD control and live evidence. */
static demo_storage_snapshot_t snapshot = {0}; /* Coherent copy via business API. */
static uint8_t data[TEST_PAGE]; /* Original payload retained through async operations. */
static uint8_t replacement[TEST_PAGE]; /* Distinct rejected-request buffer and sentinel. */
static uint8_t readback[TEST_PAGE]; /* Original read destination and comparison data. */
static uint32_t step = 0u; /* Substate in the current case. */
static uint32_t started = 0u; /* Current bounded transaction start tick. */
static uint32_t request_id = 0u; /* Expected Demo completion identifier. */
static uint8_t independent = 0u; /* NAND completed while NOR was still active. */

/** @param actual Observed value. @param expected Required value. @param id Assertion ID.
 * @return 1 on success, 0 after recording the first failure. */
static uint8_t check(int32_t actual, int32_t expected, uint32_t id)
{
    if (actual == expected) { return 1u; }
    g_fal_board_report.failure = id;
    g_fal_board_report.actual = actual;
    g_fal_board_report.expected = expected;
    g_fal_board_report.status = 3u;
    return 0u;
}

/** @return 1 if both Flash devices remain identified and error-free. */
static uint8_t healthy(void)
{
    flash_port_health_t health = {0}; /* Per-device diagnostic snapshot. */
    flash_port_health_get(0u, &health);
    if (check((int32_t)(health.chip_id == 0x00C84015u), 1, 101u) == 0u) { return 0u; }
    if (check(health.error, 0, 102u) == 0u) { return 0u; }
    flash_port_health_get(1u, &health);
    if (check((int32_t)(health.chip_id == 0xC8F1801Du), 1, 103u) == 0u) { return 0u; }
    if (check(health.error, 0, 104u) == 0u) { return 0u; }
    g_fal_board_report.corrected = health.corrected_bits;
    g_fal_board_report.bad_blocks = health.bad_blocks;
    return check((int32_t)(health.bad_blocks == 0u), 1, 105u);
}

/** @param id Next case identifier. */
static void next_case(uint32_t id)
{
    g_fal_board_report.case_id = id;
    g_fal_board_report.iteration = 0u;
    step = 0u;
    started = systick_gettime_100us();
}

static void demo_case(void)
{
    uint32_t id = g_fal_board_report.case_id; /* Current stress sequence. */
    uint32_t limit = 100u; /* Continuous and alternating cases each have 100 transactions. */
    uint8_t device = 0u; /* Physical device selector through project API. */
    if (id == 3u) { device = 1u; }
    if (id >= 4u) { device = (uint8_t)(g_fal_board_report.iteration % 2u); }
    if (id >= 5u) { limit = 2u; }
    if (step == 0u)
    {
        if (snapshot.busy == 1u) { return; }
        if (check((int32_t)demo_storage_business_request(device, DEMO_STORAGE_TEST), 1, 201u) == 0u) { return; }
        if (id == 5u)
        {
            if (check((int32_t)demo_storage_business_request((uint8_t)(1u-device), DEMO_STORAGE_TEST),
                      0, 501u) == 0u) { return; }
        }
        if (check((int32_t)demo_storage_business_read(&snapshot), 1, 202u) == 0u) { return; }
        request_id = snapshot.request_id;
        started = systick_gettime_100us();
        step = 1u;
        return;
    }
    if (snapshot.busy == 1u) { return; }
    if (check((int32_t)(snapshot.completed_id == request_id), 1, 203u) == 0u) { return; }
    if (check(snapshot.result, 0, 204u) == 0u) { return; }
    if (healthy() == 0u) { return; }
    {
        uint32_t elapsed = (systick_gettime_100us() - started) * 100u; /* Wall time in us. */
        if (elapsed > g_fal_board_report.max_demo_us) { g_fal_board_report.max_demo_us = elapsed; }
    }
    g_fal_board_report.passed[id]++;
    g_fal_board_report.iteration++;
    step = 0u;
    started = systick_gettime_100us();
    if (g_fal_board_report.iteration == limit)
    {
        if (id == 9u) { g_fal_board_report.status = 2u; }
        else { next_case(id + 1u); }
    }
}

static void nand_case(void)
{
    FLASH_PORT_STATE_E state = FLASH_PORT_BUSY; /* Actual BSP asynchronous progress. */
    if (step == 0u)
    {
        for (uint32_t i = 0u; i < TEST_PAGE; i++)
        {
            data[i] = (uint8_t)(i ^ (i >> 8u));
            replacement[i] = 0xA5u;
            readback[i] = 0u;
        }
        if (check((int32_t)flash_port_erase(1u, TEST_NAND_BASE, 131072u), 0, 601u) == 0u) { return; }
        step = 1u;
        return;
    }
    state = flash_port_state_get(1u);
    if (state == FLASH_PORT_BUSY) { return; }
    if (check((int32_t)state, (int32_t)FLASH_PORT_READY, 602u) == 0u) { return; }
    if (step == 1u)
    {
        if (check((int32_t)flash_port_program(1u, TEST_NAND_BASE, TEST_PAGE, data), 0, 603u) == 0u) { return; }
        if (check((int32_t)flash_port_program(1u, TEST_NAND_BASE, TEST_PAGE, replacement),
                  (int32_t)FLASH_PORT_INVALID_ARGUMENT, 604u) == 0u) { return; }
        step = 2u;
        return;
    }
    if (step == 2u)
    {
        if (check((int32_t)flash_port_read(1u, TEST_NAND_BASE, TEST_PAGE, readback), 0, 605u) == 0u) { return; }
        if (check((int32_t)flash_port_read(1u, TEST_NAND_BASE, TEST_PAGE, replacement),
                  (int32_t)FLASH_PORT_INVALID_ARGUMENT, 606u) == 0u) { return; }
        step = 3u;
        return;
    }
    if (check((int32_t)memcmp(data, readback, TEST_PAGE), 0, 607u) == 0u) { return; }
    for (uint32_t i = 0u; i < TEST_PAGE; i++)
    {
        if (check((int32_t)replacement[i], 0xA5, 608u) == 0u) { return; }
    }
    if (healthy() == 0u) { return; }
    g_fal_board_report.passed[6] = 2u; /* Original write/read both survived rejected requests. */
    next_case(7u);
}

static void boundary_case(void)
{
    if (check((int32_t)fal_erase(&g_demo_fal[0], DEMO_FAL_NOR_PROTECTED, 0u, 4096u),
              (int32_t)FAL_RESULT_PERMISSION_DENIED, 701u) == 0u) { return; }
    if (check((int32_t)fal_erase(&g_demo_fal[1], DEMO_FAL_NAND_PROTECTED, 0u, 131072u),
              (int32_t)FAL_RESULT_PERMISSION_DENIED, 702u) == 0u) { return; }
    if (check((int32_t)fal_write(&g_demo_fal[0], DEMO_FAL_NOR_PROTECTED, 0u, 256u, data),
              (int32_t)FAL_RESULT_PERMISSION_DENIED, 703u) == 0u) { return; }
    if (check((int32_t)fal_write(&g_demo_fal[1], DEMO_FAL_NAND_PROTECTED, 0u, TEST_PAGE, data),
              (int32_t)FAL_RESULT_PERMISSION_DENIED, 704u) == 0u) { return; }
    if (check((int32_t)fal_read(&g_demo_fal[0], DEMO_FAL_NOR_TEST, 8192u, 1u, readback),
              (int32_t)FAL_RESULT_OUT_OF_RANGE, 705u) == 0u) { return; }
    if (check((int32_t)fal_read(&g_demo_fal[1], DEMO_FAL_NAND_TEST, 262144u, 1u, readback),
              (int32_t)FAL_RESULT_OUT_OF_RANGE, 706u) == 0u) { return; }
    if (check((int32_t)fal_write(&g_demo_fal[1], DEMO_FAL_NAND_TEST, 1u, TEST_PAGE, data),
              (int32_t)FAL_RESULT_INVALID_ARGUMENT, 707u) == 0u) { return; }
    if (check((int32_t)fal_is_busy(&g_demo_fal[0]), 0, 708u) == 0u) { return; }
    if (check((int32_t)fal_is_busy(&g_demo_fal[1]), 0, 709u) == 0u) { return; }
    if (healthy() == 0u) { return; }
    g_fal_board_report.passed[7] = 7u;
    next_case(8u);
}

static void concurrent_case(void)
{
    uint8_t nor_busy = fal_is_busy(&g_demo_fal[0]); /* Current independent NOR state. */
    uint8_t nand_busy = fal_is_busy(&g_demo_fal[1]); /* Current independent NAND state. */
    if (step == 0u)
    {
        if (check((int32_t)fal_erase(&g_demo_fal[0], DEMO_FAL_NOR_TEST, 0u, 4096u),
                  (int32_t)FAL_RESULT_IN_PROGRESS, 801u) == 0u) { return; }
        step = 1u;
        return;
    }
    if (step == 1u)
    {
        if (nor_busy == 1u) { return; }
        if (check((int32_t)fal_result_get(&g_demo_fal[0]), 0, 802u) == 0u) { return; }
        if (check((int32_t)fal_write(&g_demo_fal[0], DEMO_FAL_NOR_TEST, 0u, TEST_PAGE, data),
                  (int32_t)FAL_RESULT_IN_PROGRESS, 803u) == 0u) { return; }
        if (check((int32_t)fal_read(&g_demo_fal[1], DEMO_FAL_NAND_TEST, 0u, TEST_PAGE, readback),
                  (int32_t)FAL_RESULT_IN_PROGRESS, 804u) == 0u) { return; }
        if (check((int32_t)fal_is_busy(&g_demo_fal[0]), 1, 805u) == 0u) { return; }
        if (check((int32_t)fal_is_busy(&g_demo_fal[1]), 1, 806u) == 0u) { return; }
        step = 2u;
        return;
    }
    if (step == 2u)
    {
        if ((nor_busy == 1u) && /* NOR byte-program state still owns its request. */
            (nand_busy == 0u)) /* NAND has already completed independently. */
        { independent = 1u; }
        if ((nor_busy == 1u) || /* Wait for both independently submitted requests. */
            (nand_busy == 1u)) /* No assumption about simultaneous hardware completion. */
        { return; }
        if (check((int32_t)independent, 1, 807u) == 0u) { return; }
        if (check((int32_t)fal_result_get(&g_demo_fal[0]), 0, 808u) == 0u) { return; }
        if (check((int32_t)fal_result_get(&g_demo_fal[1]), 0, 809u) == 0u) { return; }
        if (check((int32_t)memcmp(data, readback, TEST_PAGE), 0, 810u) == 0u) { return; }
        if (check((int32_t)fal_read(&g_demo_fal[0], DEMO_FAL_NOR_TEST, 0u, TEST_PAGE, readback),
                  (int32_t)FAL_RESULT_IN_PROGRESS, 811u) == 0u) { return; }
        step = 3u;
        return;
    }
    if (nor_busy == 1u) { return; }
    if (check((int32_t)fal_result_get(&g_demo_fal[0]), 0, 812u) == 0u) { return; }
    if (check((int32_t)memcmp(data, readback, TEST_PAGE), 0, 813u) == 0u) { return; }
    if (healthy() == 0u) { return; }
    g_fal_board_report.passed[8] = 1u;
    g_fal_board_report.status = 2u;
}

static void process(void)
{
    if (g_fal_board_report.status >= 2u) { return; }
    if (demo_storage_business_read(&snapshot) == 0u) { return; }
    if (g_fal_board_report.status == 0u)
    {
        uint32_t command = g_fal_board_report.command; /* Debugger publishes only while unarmed. */
        if ((command != 1u) && /* Full explicit test request. */
            (command != 2u)) /* Post-reset bounded test request. */
        { return; }
        if (snapshot.busy == 1u) { return; }
        g_fal_board_report.status = 1u;
        if (healthy() == 0u) { return; }
        g_fal_board_report.passed[1] = 1u;
        if (command == 1u) { next_case(2u); }
        else { next_case(9u); }
    }
    if ((systick_gettime_100us() - started) > TEST_TIMEOUT)
    {
        (void)check(1, 0, 999u);
        return;
    }
    if (g_fal_board_report.case_id == 6u) { nand_case(); }
    else if (g_fal_board_report.case_id == 7u) { boundary_case(); }
    else if (g_fal_board_report.case_id == 8u) { concurrent_case(); }
    else { demo_case(); }
}

REG_TASK_MS(1, process)
