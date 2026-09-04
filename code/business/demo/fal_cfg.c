// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.c
 * @brief   Register demo Flash operations, immutable partitions and independent FAL instances.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Register demo Flash operations, immutable partitions and independent FAL instances.
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
#include "flash_port.h"
#include <stddef.h>

/* Entity: 配置表拥有设备注册和分区；BSP 只认识物理地址。
 * Prior: 分区完整校验并核对实际几何后才允许业务请求。
 * Time: 唯一存储任务串行提交与推进；初始化不执行擦写。
 */
_Static_assert(DEMO_FAL_USED_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE < DEMO_FAL_NOR_CAPACITY,
               "NOR partitions must fit physical capacity");
_Static_assert(DEMO_FAL_USED_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE < DEMO_FAL_NAND_CAPACITY,
               "NAND partitions must fit physical capacity");
_Static_assert(DEMO_FAL_PARAM_BLOCKS >= 1u, "Each parameter copy needs one erase block");
_Static_assert(DEMO_FAL_TEST_BLOCKS >= 2u, "The scratch demo spans two erase blocks");
_Static_assert(DEMO_FAL_LOG_BLOCKS >= 1u && DEMO_FAL_LOG_BLOCKS <= 253u,
               "Log slots and two parameter slots must fit the scan index");
static uint8_t device_indices[DEMO_FAL_DEVICE_COUNT] = {0u, 1u}; /* FAL 回调的稳定物理索引上下文。 */
/** @param p_context 注册表上下文。 @return 物理索引，空指针返回非法哨兵。 */
static uint8_t index_get(void *p_context)
{
    if (p_context == NULL) { return DEMO_FAL_DEVICE_COUNT; }
    return *(const uint8_t *)p_context;
}
/** @param result Interface 操作结果。 @return FAL 提交结果。 */
static fal_result_t result_map(FLASH_PORT_RESULT_E result)
{
    switch (result)
    {
    case FLASH_PORT_SUCCESS: return FAL_RESULT_SUCCESS;
    case FLASH_PORT_INVALID_ARGUMENT: return FAL_RESULT_INVALID_ARGUMENT;
    default: return FAL_RESULT_DRIVER_ERROR;
    }
}
/** @param p_context 设备上下文。 @return 几何检查及芯片探测结果。 */
static fal_result_t flash_init(void *p_context)
{
    uint8_t device = index_get(p_context); /* 配置与 Interface 使用相同的物理索引。 */
    const fal_device_cfg_t *p_device = demo_fal_device_get(device); /* 已注册的几何约束。 */
    flash_port_geometry_t geometry = {0}; /* Interface 提供的真实物理几何。 */
    if (p_device == NULL) { return FAL_RESULT_INVALID_ARGUMENT; }
    if (flash_port_geometry_get(device, &geometry) == 0u) { return FAL_RESULT_CONFIG_ERROR; }
    if ((geometry.capacity != p_device->capacity) || /* 防止布局超出实际容量。 */
        (geometry.page_size != p_device->program_page_size) || /* 保持编程分页一致。 */
        (geometry.block_size != p_device->erase_block_size) || /* 保持擦除对齐一致。 */
        (geometry.program_unit != p_device->program_unit_size)) /* 遵守芯片最小写入单位。 */
    { return FAL_RESULT_CONFIG_ERROR; }
    return result_map(flash_port_init(device));
}
/** @param p_context 设备上下文。 @return FAL 设备状态。 */
static fal_device_state_t flash_state(void *p_context)
{
    switch (flash_port_state_get(index_get(p_context)))
    {
    case FLASH_PORT_READY: return FAL_DEVICE_STATE_READY;
    case FLASH_PORT_BUSY: return FAL_DEVICE_STATE_BUSY;
    default: return FAL_DEVICE_STATE_ERROR;
    }
}
/** @param p_context 设备上下文。 @param address 物理字节地址。 @param length 字节数。
 * @param p_data 接收缓冲区。 @return 请求结果。 */
static fal_result_t flash_read(void *p_context, uint32_t address, uint32_t length, uint8_t *p_data)
{
    return result_map(flash_port_read(index_get(p_context), address, length, p_data));
}
/** @param p_context 设备上下文。 @param address 物理字节地址。 @param length 字节数。
 * @param p_data 发送缓冲区。 @return 请求结果。 */
static fal_result_t flash_program(void *p_context, uint32_t address, uint32_t length, const uint8_t *p_data)
{
    return result_map(flash_port_program(index_get(p_context), address, length, p_data));
}
/** @param p_context 设备上下文。 @param address 物理字节地址。 @param length 擦除字节数。
 * @return 请求结果。 */
static fal_result_t flash_erase(void *p_context, uint32_t address, uint32_t length)
{
    return result_map(flash_port_erase(index_get(p_context), address, length));
}

static const fal_zone_cfg_t nor_zones[] = { /* NOR 分区按物理地址顺序排列。 */
    {.zone_id = DEMO_FAL_NOR_PROTECTED,
     .size = DEMO_FAL_NOR_CAPACITY - DEMO_FAL_USED_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_READ},
    {.zone_id = DEMO_FAL_NOR_PARAM_A, .size = DEMO_FAL_PARAM_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NOR_PARAM_B, .size = DEMO_FAL_PARAM_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NOR_TEST, .size = DEMO_FAL_TEST_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NOR_LOG, .size = DEMO_FAL_LOG_BLOCKS * DEMO_FAL_NOR_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
};
static const fal_zone_cfg_t nand_zones[] = { /* NAND 地址只包含主区。 */
    {.zone_id = DEMO_FAL_NAND_PROTECTED,
     .size = DEMO_FAL_NAND_CAPACITY - DEMO_FAL_USED_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_READ},
    {.zone_id = DEMO_FAL_NAND_PARAM_A, .size = DEMO_FAL_PARAM_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NAND_PARAM_B, .size = DEMO_FAL_PARAM_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NAND_TEST, .size = DEMO_FAL_TEST_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
    {.zone_id = DEMO_FAL_NAND_LOG, .size = DEMO_FAL_LOG_BLOCKS * DEMO_FAL_NAND_BLOCK_SIZE,
     .permissions = FAL_ZONE_PERMISSION_ALL},
};
static const fal_device_cfg_t devices[DEMO_FAL_DEVICE_COUNT] = { /* 设备注册表，通过适配函数注册 Interface 操作。 */
    {.device_id = 1u, .capacity = DEMO_FAL_NOR_CAPACITY, .program_page_size = 256u,
     .erase_block_size = DEMO_FAL_NOR_BLOCK_SIZE, .max_read_size = 256u,
     .p_zones = nor_zones, .zone_count = DEMO_FAL_ZONE_COUNT, .program_unit_size = 1u,
     .ops = {.p_context = &device_indices[0], .p_init = flash_init, .p_get_state = flash_state,
             .p_read = flash_read, .p_program = flash_program, .p_erase = flash_erase, .p_sync = NULL}},
    {.device_id = 2u, .capacity = DEMO_FAL_NAND_CAPACITY, .program_page_size = 2048u,
     .erase_block_size = DEMO_FAL_NAND_BLOCK_SIZE, .max_read_size = 2048u,
     .p_zones = nand_zones, .zone_count = DEMO_FAL_ZONE_COUNT, .program_unit_size = 2048u,
     .ops = {.p_context = &device_indices[1], .p_init = flash_init, .p_get_state = flash_state,
             .p_read = flash_read, .p_program = flash_program, .p_erase = flash_erase, .p_sync = NULL}},
};
fal_t g_demo_fal[DEMO_FAL_DEVICE_COUNT] = {0}; /* 运行状态与参数持久化业务独立。 */
const fal_cfg_t g_demo_fal_cfg[DEMO_FAL_DEVICE_COUNT] = { /* 两个实例独立挂载，故障互不阻塞。 */
    {.p_devices = &devices[DEMO_FAL_NOR], .device_count = 1u},
    {.p_devices = &devices[DEMO_FAL_NAND], .device_count = 1u},
};
const fal_runtime_t g_demo_fal_runtime = { /* 只描述绑定关系，生命周期和遍历由 Core 执行。 */
    .p_instances = g_demo_fal,
    .p_configs = g_demo_fal_cfg,
    .instance_count = DEMO_FAL_DEVICE_COUNT,
};

const fal_device_cfg_t *demo_fal_device_get(uint8_t device)
{
    if (device >= DEMO_FAL_DEVICE_COUNT) { return NULL; }
    return &devices[device];
}
