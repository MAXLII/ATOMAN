// SPDX-License-Identifier: MIT
/**
 * @file    test_fal_cfg.c
 * @brief   Verify standalone demo FAL configuration without persistence or data-pool modules.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Verify standalone demo FAL configuration without persistence or data-pool modules.
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
#include "fal_cfg.h"
#include "fake_flash.h"
#include "flash_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks = 0u; /* 独立配置测试的检查序号。 */
/** @param label 检查名称。 @param expected 预期值。 @param actual 实际值。 */
static void check(const char *label, int64_t expected, int64_t actual)
{
    checks++;
    (void)printf("CHECK %03u %-36s expected=%lld actual=%lld %s\n",
        checks, label, (long long)expected, (long long)actual, expected == actual ? "PASS" : "FAIL");
    if (expected != actual) { exit(1); }
}
/** @param device 实例索引。 @return 完整操作结果。 */
static fal_result_t finish(uint8_t device)
{
    uint32_t steps = 0u; /* 异步推进预算。 */
    while ((fal_is_busy(&g_demo_fal[device]) == 1u) && (steps < 100u))
    {
        (void)fal_runtime_process(&g_demo_fal_runtime); steps++;
    }
    check("direct request completed", 0, fal_is_busy(&g_demo_fal[device]));
    return fal_result_get(&g_demo_fal[device]);
}
int main(void)
{
    uint8_t source[2048] = {0}; /* 在整个异步请求期间保留的源缓冲。 */
    uint8_t destination[2048] = {0}; /* 读回缓冲。 */
    (void)printf("CASE standalone fal_cfg + fal_core + Interface + fake BSP; no storage service or data pool\n");
    fake_reset();
    check("core initializes runtime", FAL_RESULT_SUCCESS, fal_runtime_init(&g_demo_fal_runtime));
    {
        fal_runtime_t invalid = g_demo_fal_runtime; /* 验证描述符错误不进入硬件。 */
        check("null runtime init", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_init(NULL));
        check("null runtime process", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_process(NULL));
        check("null runtime mount", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_mount(NULL, 0u));
        invalid.p_instances = NULL;
        check("missing instances rejected", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_init(&invalid));
        invalid = g_demo_fal_runtime;
        invalid.p_configs = NULL;
        check("missing configs rejected", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_process(&invalid));
        invalid = g_demo_fal_runtime;
        invalid.instance_count = 0u;
        check("empty runtime rejected", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_init(&invalid));
        check("empty mount rejected", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_mount(&invalid, 0u));
    }
    check("init performs no mutation", 0, fake_writes());
    for (uint8_t device = 0u; device < DEMO_FAL_DEVICE_COUNT; device++)
    {
        const fal_device_cfg_t *p_device = demo_fal_device_get(device); /* 真实 demo 注册表。 */
        fal_zone_id_t test_zone = p_device->p_zones[3].zone_id; /* 真实测试分区。 */
        fal_zone_id_t protected_zone = p_device->p_zones[0].zone_id; /* 真实保护分区。 */
        uint32_t page = p_device->program_page_size; /* 写入长度。 */
        uint32_t offset = 0u; /* 累计地址。 */
        (void)printf("INPUT device=%u page=%lu block=%lu test_zone=%u\n", (unsigned)device,
            (unsigned long)page, (unsigned long)p_device->erase_block_size, (unsigned)test_zone);
        check("registered instance idle", FAL_STATE_IDLE, fal_state_get(&g_demo_fal[device]));
        check("five partitions", 5, p_device->zone_count);
        for (uint16_t z = 0u; z < p_device->zone_count; z++)
        {
            check("partition aligned", 0, offset % p_device->erase_block_size);
            offset += p_device->p_zones[z].size;
        }
        check("partitions cover capacity", p_device->capacity, offset);
        check("protected write rejected", FAL_RESULT_PERMISSION_DENIED,
            fal_write(&g_demo_fal[device], protected_zone, 0u, page, source));
        check("protected erase rejected", FAL_RESULT_PERMISSION_DENIED,
            fal_erase(&g_demo_fal[device], protected_zone, 0u, p_device->erase_block_size));
        check("direct erase accepted", FAL_RESULT_IN_PROGRESS,
            fal_erase(&g_demo_fal[device], test_zone, 0u, p_device->erase_block_size));
        check("remount preserves pending request", FAL_RESULT_BUSY, fal_runtime_mount(&g_demo_fal_runtime, device));
        {
            uint32_t before = g_demo_fal[0].physical_address; /* 全组预检查保护前面的空闲实例。 */
            check("group init rejects active request", FAL_RESULT_BUSY, fal_runtime_init(&g_demo_fal_runtime));
            check("group preflight preserves state", before, g_demo_fal[0].physical_address);
        }
        check("direct erase succeeds", 0, finish(device));
        for (uint32_t i = 0u; i < page; i++) { source[i] = (uint8_t)(i ^ (i >> 8u)); }
        check("direct write accepted", FAL_RESULT_IN_PROGRESS,
            fal_write(&g_demo_fal[device], test_zone, 0u, page, source));
        check("direct write succeeds", 0, finish(device));
        check("direct read accepted", FAL_RESULT_IN_PROGRESS,
            fal_read(&g_demo_fal[device], test_zone, 0u, page, destination));
        check("direct read succeeds", 0, finish(device));
        check("direct data comparison", 0, memcmp(source, destination, page));
        check("out-of-range rejected", FAL_RESULT_OUT_OF_RANGE,
            fal_read(&g_demo_fal[device], test_zone, p_device->p_zones[3].size, 1u, destination));
    }
    check("protected prefix never reached BSP", 0, fake_prefix_writes());
    fake_present(0u, 0u);
    check("runtime reports mount failure", FAL_RESULT_DRIVER_ERROR, fal_runtime_init(&g_demo_fal_runtime));
    check("runtime still schedules peers", FAL_RESULT_SUCCESS, fal_runtime_process(&g_demo_fal_runtime));
    check("absent NOR fails explicitly", FAL_STATE_ERROR, fal_state_get(&g_demo_fal[DEMO_FAL_NOR]));
    check("NAND remains usable", FAL_STATE_IDLE, fal_state_get(&g_demo_fal[DEMO_FAL_NAND]));
    check("invalid config index rejected", FAL_RESULT_INVALID_ARGUMENT, fal_runtime_mount(&g_demo_fal_runtime, 2u));
    (void)printf("SUMMARY standalone checks=%u passed=%u failed=0\n", checks, checks);
    return 0;
}
