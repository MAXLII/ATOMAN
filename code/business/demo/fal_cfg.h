// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.h
 * @brief   Define demo Flash device registration, partition identifiers and FAL instances.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Define demo Flash device registration, partition identifiers and FAL instances.
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
#ifndef DEMO_FAL_CFG_H
#define DEMO_FAL_CFG_H
#include "fal_core.h"
#define DEMO_FAL_DEVICE_COUNT 2u /* NOR 与 NAND 的独立实例数量。 */
#define DEMO_FAL_ZONE_COUNT 5u /* 每颗设备的分区数量。 */
#define DEMO_FAL_NOR_CAPACITY (2u * 1024u * 1024u) /* NOR 字节容量。 */
#define DEMO_FAL_NAND_CAPACITY (128u * 1024u * 1024u) /* NAND 主区字节容量。 */
#define DEMO_FAL_NOR_BLOCK_SIZE 4096u /* NOR 分区配置的擦除单位。 */
#define DEMO_FAL_NAND_BLOCK_SIZE 131072u /* NAND 分区配置的擦除单位。 */
#define DEMO_FAL_PARAM_BLOCKS 1u /* 每份参数占用的擦除块数。 */
#define DEMO_FAL_TEST_BLOCKS 2u /* 跨块测试区域。 */
#define DEMO_FAL_LOG_BLOCKS 4u /* 循环日志的槽位数。 */
#define DEMO_FAL_USED_BLOCKS (2u * DEMO_FAL_PARAM_BLOCKS + DEMO_FAL_TEST_BLOCKS + DEMO_FAL_LOG_BLOCKS) /* 可写块总数。 */
typedef enum
{
    DEMO_FAL_NOR = 0, /* NOR 实例索引。 */
    DEMO_FAL_NAND = 1 /* NAND 实例索引。 */
} DEMO_FAL_DEVICE_E;
typedef enum
{
    DEMO_FAL_NOR_PROTECTED = 1, /* NOR 前部只读区域。 */
    DEMO_FAL_NOR_PARAM_A, /* NOR 参数 A。 */
    DEMO_FAL_NOR_PARAM_B, /* NOR 参数 B。 */
    DEMO_FAL_NOR_TEST, /* NOR 测试区域。 */
    DEMO_FAL_NOR_LOG, /* NOR 日志区域。 */
    DEMO_FAL_NAND_PROTECTED = 101, /* NAND 前部只读区域。 */
    DEMO_FAL_NAND_PARAM_A, /* NAND 参数 A。 */
    DEMO_FAL_NAND_PARAM_B, /* NAND 参数 B。 */
    DEMO_FAL_NAND_TEST, /* NAND 测试区域。 */
    DEMO_FAL_NAND_LOG /* NAND 日志区域。 */
} DEMO_FAL_ZONE_E;
extern fal_t g_demo_fal[DEMO_FAL_DEVICE_COUNT]; /* Demo 直接交给 fal_read/write/erase 的实例。 */
extern const fal_cfg_t g_demo_fal_cfg[DEMO_FAL_DEVICE_COUNT]; /* 每个实例对应的设备注册配置。 */
extern const fal_runtime_t g_demo_fal_runtime; /* 交给 Core 管理的实例与配置绑定。 */
/** @param device 实例索引。 @return 指向已注册设备的只读指针，非法索引返回 NULL。 */
const fal_device_cfg_t *demo_fal_device_get(uint8_t device);
#endif
