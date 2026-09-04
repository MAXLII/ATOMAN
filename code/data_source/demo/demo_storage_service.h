// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_service.h
 * @brief   Expose periodic demo persistence service lifecycle.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Expose periodic demo persistence service lifecycle.
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
#ifndef DEMO_STORAGE_SERVICE_H
#define DEMO_STORAGE_SERVICE_H
/** Initialize after fal_runtime_init(); call before periodic storage dispatch. */
void demo_storage_service_init(void);
/** Advance the storage transaction after fal_runtime_process(), in the same task; not ISR-safe. */
void demo_storage_service_process(void);
#endif
