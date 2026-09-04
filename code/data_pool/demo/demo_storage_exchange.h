// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_exchange.h
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
#ifndef DEMO_STORAGE_EXCHANGE_H
#define DEMO_STORAGE_EXCHANGE_H
#include "demo_storage_pool.h"
/** @param p_request 冻结的请求输出。 @return 1 有新请求，0 无请求或忙。 */
uint8_t demo_storage_exchange_take(demo_storage_request_t *p_request);
/** @param id 完成的请求编号。 @param result 操作结果。
 * @param p_restored 可选的恢复参数。 @param p_devices 两颗设备状态。
 * @return 1 发布成功，0 锁忙或请求不匹配，应在后续任务重试。 */
uint8_t demo_storage_exchange_finish(uint32_t id, int32_t result,
    const demo_storage_params_t *p_restored, const demo_storage_device_t *p_devices);
/** @param p_devices 两颗设备的启动探测结果。 @return 1 发布成功，0 锁忙。 */
uint8_t demo_storage_exchange_probe(const demo_storage_device_t *p_devices);
/** @param p_defaults 数据源提供的默认参数。 @return 1 占有启动事务，0 忙。 */
uint8_t demo_storage_exchange_boot_begin(const demo_storage_params_t *p_defaults);
#endif
