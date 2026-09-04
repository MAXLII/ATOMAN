// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_pool.c
 * @brief   Own demo storage parameters, command mailbox and result snapshots.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Own demo storage parameters, command mailbox and result snapshots.
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
#include "demo_storage_business.h"
#include "demo_storage_exchange.h"
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
static atomic_flag pool_lock = ATOMIC_FLAG_INIT; /* 任务间非阻塞互斥，ISR 不调用。 */
static demo_storage_snapshot_t pool = {0}; /* 唯一共享存储数据对象，静态零初始化。 */
static demo_storage_request_t request = {0}; /* 同一对象的命令槽，消费期间冻结。 */
static uint8_t taken = 0u; /* 当前命令已经被数据源领取。 */

static uint8_t enter(void)
{
    return (uint8_t)(!atomic_flag_test_and_set_explicit(&pool_lock, memory_order_acquire));
}
static void leave(void) { atomic_flag_clear_explicit(&pool_lock, memory_order_release); }

uint8_t demo_storage_business_configure(const demo_storage_params_t *p_params)
{
    if (p_params == NULL) { return 0u; }
    if (enter() == 0u) { return 0u; }
    if (pool.busy == 1u) { leave(); return 0u; }
    pool.params = *p_params;
    leave();
    return 1u;
}
uint8_t demo_storage_business_request(uint8_t device, DEMO_STORAGE_COMMAND_E command)
{
    if ((device >= 2u) || (command < DEMO_STORAGE_SCAN) || (command > DEMO_STORAGE_DEFAULTS)) { return 0u; }
    if (enter() == 0u) { return 0u; }
    if (pool.busy == 1u) { leave(); return 0u; }
    pool.request_id++;
    if (pool.request_id == 0u) { pool.request_id = 1u; }
    request.params = pool.params;
    request.id = pool.request_id;
    request.command = command;
    request.device = device;
    pool.busy = 1u;
    taken = 0u;
    leave();
    return 1u;
}
uint8_t demo_storage_business_read(demo_storage_snapshot_t *p_snapshot)
{
    if ((p_snapshot == NULL) || (enter() == 0u)) { return 0u; }
    *p_snapshot = pool;
    leave();
    return 1u;
}
uint8_t demo_storage_exchange_take(demo_storage_request_t *p_request)
{
    if ((p_request == NULL) || (enter() == 0u)) { return 0u; }
    if ((pool.busy == 0u) || (taken == 1u)) { leave(); return 0u; }
    *p_request = request;
    taken = 1u;
    leave();
    return 1u;
}
uint8_t demo_storage_exchange_finish(uint32_t id, int32_t result,
    const demo_storage_params_t *p_restored, const demo_storage_device_t *p_devices)
{
    if ((p_devices == NULL) || (enter() == 0u)) { return 0u; }
    if ((pool.busy == 0u) || (taken == 0u) || (request.id != id)) { leave(); return 0u; }
    if (p_restored != NULL) { pool.params = *p_restored; }
    (void)memcpy(pool.devices, p_devices, sizeof(pool.devices));
    pool.result = result;
    pool.completed_id = id;
    pool.busy = 0u;
    taken = 0u;
    leave();
    return 1u;
}
uint8_t demo_storage_exchange_probe(const demo_storage_device_t *p_devices)
{
    if ((p_devices == NULL) || (enter() == 0u)) { return 0u; }
    (void)memcpy(pool.devices, p_devices, sizeof(pool.devices));
    leave();
    return 1u;
}
uint8_t demo_storage_exchange_boot_begin(const demo_storage_params_t *p_defaults)
{
    if ((p_defaults == NULL) || (enter() == 0u)) { return 0u; }
    if (pool.busy == 1u) { leave(); return 0u; }
    pool.params = *p_defaults;
    request.id = 0u;
    pool.busy = 1u;
    taken = 1u;
    leave();
    return 1u;
}
