// SPDX-License-Identifier: MIT
/**
 * @file    demo_fal.c
 * @brief   Register task-context Flash demo control and data-pool observation.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Register task-context Flash demo control and data-pool observation.
 *          - Keep operations bounded and report invalid requests.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Task-context API; not ISR-safe.
 *          - Hardware access is confined to the platform BSP.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "demo_fal.h"
#include "demo_storage_business.h"
#include "shell.h"
#include "section.h"
#include <stddef.h>
static uint32_t device = 0u; /* 用户选中的 NOR/NAND。 */
static uint32_t edit_input_mv = 1000u; /* 尚未发布的数据池候选输入。 */
static uint32_t edit_gain_milli = 1000u; /* 尚未发布的数据池候选增益。 */
static uint32_t edit_threshold_mv = 1500u; /* 尚未发布的数据池候选阈值。 */
static uint32_t test_armed = 0u; /* 一次性测试擦写确认，执行后清零。 */
static uint32_t output_mv = 0u; /* 根据数据池参数实时计算的演示输出。 */
static uint32_t threshold_hit = 0u; /* 演示输出达到阈值。 */

static void calculate(void)
{
    demo_storage_snapshot_t snapshot = {0}; /* 数据池一致快照。 */
    if (demo_storage_business_read(&snapshot) == 0u) { return; }
    if ((snapshot.params.input_mv > 100000u) || (snapshot.params.gain_milli > 10000u) ||
        (snapshot.params.threshold_mv > 1000000u))
    { output_mv = 0u; threshold_hit = 0u; return; }
    output_mv = (uint32_t)(((uint64_t)snapshot.params.input_mv * snapshot.params.gain_milli) / 1000u);
    threshold_hit = (output_mv >= snapshot.params.threshold_mv) ? 1u : 0u;
}
static void show(DEC_MY_PRINTF)
{
    demo_storage_snapshot_t snapshot = {0}; /* 数据池和设备状态。 */
    static const char *const names[5] = {"protected", "param_a", "param_b", "test", "log"}; /* 分区名称。 */
    if ((my_printf == NULL) || (my_printf->my_printf == NULL)) { return; }
    if (demo_storage_business_read(&snapshot) == 0u) { my_printf->my_printf("FAL pool busy\r\n"); return; }
    my_printf->my_printf("FAL busy=%u request=%lu completed=%lu result=%ld RAM: input=%lu gain=%lu threshold=%lu\r\n",
        (unsigned)snapshot.busy, (unsigned long)snapshot.request_id, (unsigned long)snapshot.completed_id,
        (long)snapshot.result, (unsigned long)snapshot.params.input_mv,
        (unsigned long)snapshot.params.gain_milli, (unsigned long)snapshot.params.threshold_mv);
    my_printf->my_printf("demo output_mv=%lu threshold_hit=%lu\r\n",
        (unsigned long)output_mv, (unsigned long)threshold_hit);
    for (uint32_t i = 0u; i < 2u; i++)
    {
        const demo_storage_device_t *p_view = &snapshot.devices[i]; /* 当前介质快照。 */
        my_printf->my_printf("dev=%lu id=%08lX online=%u valid=%u seq=%lu page=%lu block=%lu error=%ld ecc=%lu bad=%lu\r\n",
            (unsigned long)i, (unsigned long)p_view->chip_id, (unsigned)p_view->online, (unsigned)p_view->valid,
            (unsigned long)p_view->sequence, (unsigned long)p_view->page_size, (unsigned long)p_view->block_size,
            (long)p_view->driver_error, (unsigned long)p_view->corrected_bits, (unsigned long)p_view->bad_blocks);
        my_printf->my_printf("  log seq=%lu input=%lu gain=%lu threshold=%lu\r\n",
            (unsigned long)p_view->log_sequence, (unsigned long)p_view->last_log.input_mv,
            (unsigned long)p_view->last_log.gain_milli, (unsigned long)p_view->last_log.threshold_mv);
        for (uint32_t z = 0u; z < 5u; z++)
        {
            my_printf->my_printf("  %s offset=%08lX size=%lu write=%u\r\n", names[z],
                (unsigned long)p_view->zones[z].offset, (unsigned long)p_view->zones[z].size,
                (unsigned)p_view->zones[z].writable);
        }
    }
}
static void apply(DEC_MY_PRINTF)
{
    demo_storage_params_t params = { /* 一次性发布全部参数，不暴露池内地址。 */
        .input_mv = edit_input_mv, .gain_milli = edit_gain_milli, .threshold_mv = edit_threshold_mv
    };
    uint8_t accepted = demo_storage_business_configure(&params); /* 发布结果。 */
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    { my_printf->my_printf("FAL apply accepted=%u\r\n", (unsigned)accepted); }
}
static void report_request(DEMO_STORAGE_COMMAND_E command, DEC_MY_PRINTF)
{
    uint8_t accepted = demo_storage_business_request((uint8_t)device, command); /* 请求只入池，不在 Shell 擦写。 */
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    { my_printf->my_printf("FAL queued=%u; use FAL_INFO for completion\r\n", (unsigned)accepted); }
}
static void scan(DEC_MY_PRINTF) { report_request(DEMO_STORAGE_SCAN, my_printf); }
static void save(DEC_MY_PRINTF) { report_request(DEMO_STORAGE_SAVE, my_printf); }
static void load(DEC_MY_PRINTF) { report_request(DEMO_STORAGE_LOAD, my_printf); }
static void log_append(DEC_MY_PRINTF) { report_request(DEMO_STORAGE_LOG, my_printf); }
static void defaults(DEC_MY_PRINTF) { report_request(DEMO_STORAGE_DEFAULTS, my_printf); }
static void test(DEC_MY_PRINTF)
{
    if (test_armed == 1u) { test_armed = 0u; report_request(DEMO_STORAGE_TEST, my_printf); }
    else if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    { my_printf->my_printf("Set FAL_TEST_ARM=1 to erase TEST partitions only\r\n"); }
}
REG_TASK_MS(10, calculate)
REG_SHELL_VAR(FAL_DEVICE, device, SHELL_UINT32, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FAL_EDIT_INPUT_MV, edit_input_mv, SHELL_UINT32, 100000u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FAL_EDIT_GAIN_MILLI, edit_gain_milli, SHELL_UINT32, 10000u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FAL_EDIT_THRESHOLD_MV, edit_threshold_mv, SHELL_UINT32, 1000000u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FAL_TEST_ARM, test_armed, SHELL_UINT32, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_CMD(FAL_INFO, show)
REG_SHELL_CMD(FAL_APPLY, apply)
REG_SHELL_CMD(FAL_SCAN, scan)
REG_SHELL_CMD(FAL_SAVE, save)
REG_SHELL_CMD(FAL_LOAD, load)
REG_SHELL_CMD(FAL_LOG_APPEND, log_append)
REG_SHELL_CMD(FAL_DEFAULTS, defaults)
REG_SHELL_CMD(FAL_TEST, test)
