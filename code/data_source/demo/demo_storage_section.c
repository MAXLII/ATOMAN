// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_section.c
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
#include "demo_storage_service.h"
#include "fal_cfg.h"
#include "section.h"

static void storage_init(void)
{
    (void)fal_runtime_init(&g_demo_fal_runtime); /* Core 挂载实例，各实例保留自己的初始化结果。 */
}

static void storage_process(void)
{
    (void)fal_runtime_process(&g_demo_fal_runtime); /* 由 Core 遍历并推进各实例的请求状态。 */
    demo_storage_service_process(); /* 执行参数、日志与数据池事务。 */
}

REG_INIT(0, storage_init)
REG_INIT(1, demo_storage_service_init)
REG_TASK_MS(1, storage_process)
