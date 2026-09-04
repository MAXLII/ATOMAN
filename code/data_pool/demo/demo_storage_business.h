// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_business.h
 * @brief   Define the private demo storage data-pool boundary and typed snapshots.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Define the private demo storage data-pool boundary and typed snapshots.
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
#ifndef DEMO_STORAGE_BUSINESS_H
#define DEMO_STORAGE_BUSINESS_H
#include "demo_storage_pool.h"
/** @param p_params 待提交 RAM 参数。 @return 1 成功，0 忙或值非法。 */
uint8_t demo_storage_business_configure(const demo_storage_params_t *p_params);
/** @param device 介质编号。 @param command 请求动作。 @return 1 接受，0 忙或参数非法。 */
uint8_t demo_storage_business_request(uint8_t device, DEMO_STORAGE_COMMAND_E command);
/** @param p_snapshot 快照输出，失败时保留原值。 @return 1 成功，0 忙或空指针。 */
uint8_t demo_storage_business_read(demo_storage_snapshot_t *p_snapshot);
#endif
