// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_pool.h
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
#ifndef DEMO_STORAGE_POOL_H
#define DEMO_STORAGE_POOL_H
#include <stdint.h>
typedef enum
{
    DEMO_STORAGE_SCAN = 1, /* Read-only probe and parameter scan. */
    DEMO_STORAGE_SAVE, /* Save current RAM settings to selected medium. */
    DEMO_STORAGE_LOAD, /* Load newest valid settings from selected medium. */
    DEMO_STORAGE_TEST, /* Destructive scratch-only read/write comparison. */
    DEMO_STORAGE_LOG, /* Append one settings snapshot to the bounded log ring. */
    DEMO_STORAGE_DEFAULTS /* Restore RAM defaults only. */
} DEMO_STORAGE_COMMAND_E;
typedef struct
{
    uint32_t input_mv; /* 演示输入电压，0..100000 mV。 */
    uint32_t gain_milli; /* 演示增益，0..10000，1000 表示 1 倍。 */
    uint32_t threshold_mv; /* 演示比较阈值，0..1000000 mV。 */
} demo_storage_params_t;
typedef struct
{
    uint32_t offset; /* 设备内字节偏移。 */
    uint32_t size; /* 区域字节数。 */
    uint8_t writable; /* 区域允许擦写。 */
} demo_storage_zone_t;
typedef struct
{
    uint32_t chip_id; /* 实际读取到的芯片 ID。 */
    uint32_t page_size; /* 写入页大小。 */
    uint32_t block_size; /* 最小擦除块大小。 */
    demo_storage_zone_t zones[5]; /* 保护区、参数 A、参数 B、测试区、日志区。 */
    uint32_t sequence; /* 最新有效参数记录序号。 */
    uint32_t log_sequence; /* 最近一次完整日志的序号，0 表示无记录。 */
    demo_storage_params_t last_log; /* 最近一次完整日志的参数快照。 */
    uint32_t corrected_bits; /* 本次启动累计 ECC 纠正位数。 */
    uint32_t bad_blocks; /* 本次启动发现的坏块数。 */
    int32_t driver_error; /* BSP 最近的详细错误。 */
    uint8_t online; /* 设备已挂载。 */
    uint8_t valid; /* 存在有效参数副本。 */
} demo_storage_device_t;
typedef struct
{
    demo_storage_params_t params; /* 当前共享 RAM 参数。 */
    demo_storage_device_t devices[2]; /* NOR/NAND 观测结果。 */
    uint32_t request_id; /* 已接受命令的递增序号。 */
    uint32_t completed_id; /* 已完成命令序号。 */
    int32_t result; /* 0 成功，-20 无记录，-21 校验失败，-22 离线/扫描失败，-23 扫描不完整禁止写入。 */
    uint8_t busy; /* 存储事务已排队或正在执行。 */
} demo_storage_snapshot_t;
typedef struct
{
    demo_storage_params_t params; /* 请求接受时冻结的参数。 */
    DEMO_STORAGE_COMMAND_E command; /* 要执行的动作。 */
    uint32_t id; /* 请求序号。 */
    uint8_t device; /* 0 NOR，1 NAND。 */
} demo_storage_request_t;
#endif
