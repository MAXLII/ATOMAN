// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_service.c
 * @brief   Manage bounded demo partitions, durable parameter records and data-pool exchanges.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Manage bounded demo partitions, durable parameter records and data-pool exchanges.
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
#include "demo_storage_exchange.h"
#include "demo_storage_record.h"
#include "flash_port.h"
#include "flash_integrity.h"
#include <stddef.h>
#include <string.h>
typedef enum
{
    STORAGE_IDLE,
    STORAGE_SCAN_DATA,
    STORAGE_SCAN_COMMIT,
    STORAGE_DECIDE,
    STORAGE_ERASE,
    STORAGE_WRITE,
    STORAGE_VERIFY,
    STORAGE_COMMIT,
    STORAGE_VERIFY_COMMIT,
    STORAGE_FINISH
} STORAGE_PHASE_E;
static const fal_device_cfg_t *p_devices[DEMO_FAL_DEVICE_COUNT]; /* 只读引用 demo fal_cfg 的注册配置。 */
static demo_storage_device_t views[2];       /* 对外发布的设备状态。 */
static demo_storage_params_t recovered[2];   /* 扫描得到的持久化候选值，并非共享 RAM 真值。 */
static demo_storage_request_t request = {0}; /* 数据池领取后冻结的命令。 */
static const demo_storage_params_t defaults =
    {/* 默认值属于数据源，不属于数据池。 */
     .input_mv = 1000u,
     .gain_milli = 1000u,
     .threshold_mv = 1500u};
static STORAGE_PHASE_E phase = STORAGE_IDLE;        /* 事务状态。 */
static uint8_t data_page[2048];                     /* 编码页或扫描页。 */
static uint8_t commit_page[2048];                   /* 独立提交页。 */
static uint8_t verify_page[2048];                   /* 写后读回缓冲。 */
static uint8_t active_slot[2];                      /* 当前有效参数 A/B。 */
static uint8_t log_slot[2];                         /* 最近有效日志所在块。 */
static uint32_t log_sequence[2];                    /* 最近有效日志序号。 */
static uint8_t scan_slot = 0u;                      /* 0/1 参数副本，2..5 日志块。 */
static uint8_t scan_failed = 0u;                    /* 扫描 I/O 不完整时禁止覆盖未知的持久化记录。 */
static uint8_t waiting = 0u;                        /* FAL 已接受当前子操作。 */
static uint8_t booting = 0u;                        /* 启动只读扫描占有数据池命令槽。 */
static uint8_t restore = 0u;                        /* 完成时是否发布恢复参数。 */
static uint8_t target_zone = 0u;                    /* 待写区域索引。 */
static uint32_t target_offset = 0u;                 /* 区域内记录偏移。 */
static uint32_t next_sequence = 0u;                 /* 本次写入序号。 */
static int32_t outcome = 0;                         /* 本次事务结果。 */
static fal_result_t io_result = FAL_RESULT_SUCCESS; /* 最近的 FAL 子操作结果。 */

static void health_update(uint8_t device)
{
    flash_port_health_t health = {0}; /* Interface 获取的硬件诊断快照。 */
    flash_port_health_get(device, &health);
    views[device].chip_id = health.chip_id;
    views[device].corrected_bits = health.corrected_bits;
    views[device].bad_blocks = health.bad_blocks;
    if (health.error != 0)
    {
        views[device].driver_error = health.error;
    }
}
static void mount(uint8_t device)
{
    fal_result_t result = fal_runtime_mount(&g_demo_fal_runtime, device); /* Core 挂载指定实例，保留其他介质。 */
    views[device].online = (result == FAL_RESULT_SUCCESS) ? 1u : 0u;
    health_update(device);
}
static void scan_begin(void)
{
    scan_slot = 0u;
    scan_failed = 0u;
    views[request.device].valid = 0u;
    views[request.device].sequence = 0u;
    log_sequence[request.device] = 0u;
    views[request.device].log_sequence = 0u;
    (void)memset(&views[request.device].last_log, 0, sizeof(views[request.device].last_log));
    log_slot[request.device] = (uint8_t)(DEMO_FAL_LOG_BLOCKS - 1u);
    phase = STORAGE_SCAN_DATA;
}
void demo_storage_service_init(void)
{
    /* Initialization owns the pool before touching worker state or devices. */
    if (demo_storage_exchange_boot_begin(&defaults) == 0u)
    {
        return;
    }
    (void)memset(views, 0, sizeof(views));
    for (uint8_t d = 0u; d < DEMO_FAL_DEVICE_COUNT; d++)
    {
        uint32_t offset = 0u; /* 仅为数据池观测计算分区起点。 */
        p_devices[d] = demo_fal_device_get(d); /* 引用独立配置模块，不构造设备或分区。 */
        for (uint8_t z = 0u; z < DEMO_FAL_ZONE_COUNT; z++)
        {
            const fal_zone_cfg_t *p_zone = &p_devices[d]->p_zones[z]; /* 已注册分区。 */
            views[d].zones[z].offset = offset;
            views[d].zones[z].size = p_zone->size;
            views[d].zones[z].writable = (uint8_t)((p_zone->permissions & FAL_ZONE_PERMISSION_WRITE) != 0u);
            offset += p_zone->size;
        }
        views[d].page_size = p_devices[d]->program_page_size;
        views[d].block_size = p_devices[d]->erase_block_size;
        views[d].online = (uint8_t)(fal_state_get(&g_demo_fal[d]) == FAL_STATE_IDLE);
        health_update(d);
    }
    waiting = 0u;
    restore = 0u;
    outcome = 0;
    request.id = 0u;
    request.device = 0u;
    request.command = DEMO_STORAGE_SCAN;
    booting = 1u;
    scan_begin();
}
static uint8_t transfer(uint8_t op, uint8_t zone, uint32_t offset, uint32_t length, uint8_t *p_buffer)
{
    fal_t *p_fal = &g_demo_fal[request.device]; /* 当前事务唯一拥有的实例。 */
    if (waiting == 0u)
    {
        fal_zone_id_t id = p_devices[request.device]->p_zones[zone].zone_id; /* 选定介质内全局唯一分区 ID。 */
        if (op == 1u)
        {
            io_result = fal_read(p_fal, id, offset, length, p_buffer);
        }
        else if (op == 2u)
        {
            io_result = fal_write(p_fal, id, offset, length, p_buffer);
        }
        else
        {
            io_result = fal_erase(p_fal, id, offset, length);
        }
        if (io_result == FAL_RESULT_IN_PROGRESS)
        {
            waiting = 1u;
            return 0u;
        }
        return 1u;
    }
    if (fal_is_busy(p_fal) == 1u)
    {
        return 0u;
    }
    io_result = fal_result_get(p_fal);
    waiting = 0u;
    return 1u;
}
static void scan_advance(void)
{
    if (io_result != FAL_RESULT_SUCCESS)
    {
        scan_failed = 1u;
    }
    if (io_result == FAL_RESULT_DRIVER_ERROR)
    {
        health_update(request.device);
        mount(request.device); /* 只读重置，允许另一个固定副本继续被检查。 */
    }
    scan_slot++;
    phase = (scan_slot < DEMO_FAL_LOG_BLOCKS + 2u) ? STORAGE_SCAN_DATA : STORAGE_DECIDE;
}

static void decide(void)
{
    uint8_t d = request.device; /* 选中介质。 */
    if (booting == 1u)
    {
        if (d == 0u)
        {
            request.device = 1u;
            scan_begin();
            return;
        }
        request.device = 0u; /* 自动恢复只选 NOR；NAND 仅作为显式加载来源。 */
        restore = views[0].valid;
        phase = STORAGE_FINISH;
        return;
    }
    if (request.command == DEMO_STORAGE_SCAN)
    {
        outcome = ((views[d].online == 0u) || (scan_failed == 1u)) ? -22 : 0;
        phase = STORAGE_FINISH;
        return;
    }
    if (request.command == DEMO_STORAGE_LOAD)
    {
        restore = views[d].valid;
        outcome = (restore == 1u) ? 0 : ((views[d].online == 0u) ? -22 : -20);
        phase = STORAGE_FINISH;
        return;
    }
    if (views[d].online == 0u)
    {
        outcome = -22;
        phase = STORAGE_FINISH;
        return;
    }
    if (scan_failed == 1u)
    {
        outcome = -23;
        phase = STORAGE_FINISH;
        return;
    }
    if (demo_storage_record_params_valid(&request.params) == 0u)
    {
        outcome = FAL_RESULT_INVALID_ARGUMENT;
        phase = STORAGE_FINISH;
        return;
    }
    target_offset = 0u;
    if (request.command == DEMO_STORAGE_SAVE)
    {
        target_zone = (views[d].valid == 1u) ? (uint8_t)(2u - active_slot[d]) : 1u;
        next_sequence = views[d].sequence + 1u;
    }
    else
    {
        target_zone = 4u;
        target_offset = ((uint32_t)(log_slot[d] + 1u) % DEMO_FAL_LOG_BLOCKS) * p_devices[d]->erase_block_size;
        next_sequence = log_sequence[d] + 1u;
    }
    if (next_sequence == 0u)
    {
        next_sequence = 1u;
    }
    demo_storage_record_encode(data_page, commit_page, p_devices[d]->program_page_size,
                               (request.command == DEMO_STORAGE_SAVE) ? 1u : 2u, next_sequence, &request.params);
    phase = STORAGE_ERASE;
}

void demo_storage_service_process(void)
{
    uint8_t d = request.device;                   /* 当前事务介质，初始化为合法索引。 */
    uint32_t page = p_devices[d]->program_page_size; /* 本次操作的完整页大小。 */
    uint32_t block = p_devices[d]->erase_block_size; /* 本次操作的擦除粒度。 */
    if (phase == STORAGE_IDLE)
    {
        if (demo_storage_exchange_take(&request) == 0u)
        {
            return;
        }
        d = request.device;
        page = p_devices[d]->program_page_size;
        block = p_devices[d]->erase_block_size;
        outcome = 0;
        restore = 0u;
        waiting = 0u;
        views[d].driver_error = 0;
        if (request.command == DEMO_STORAGE_DEFAULTS)
        {
            recovered[d] = defaults;
            restore = 1u;
            phase = STORAGE_FINISH;
        }
        else if (request.command == DEMO_STORAGE_TEST)
        {
            if (views[d].online == 0u)
            {
                outcome = -22;
                phase = STORAGE_FINISH;
            }
            else
            {
                target_zone = 3u;
                target_offset = block - page; /* 跨越两个测试块的页边界。 */
                next_sequence = request.id;
                demo_storage_record_encode(data_page, commit_page, page, 3u, next_sequence, &defaults);
                for (uint32_t i = 32u; i < page; i++)
                {
                    data_page[i] = (uint8_t)(i ^ (i >> 8u));
                }
                phase = STORAGE_ERASE;
            }
        }
        else
        {
            mount(d);
            scan_begin();
        }
        return;
    }
    if ((phase == STORAGE_SCAN_DATA) && (views[d].online == 0u))
    {
        phase = STORAGE_DECIDE;
    }
    if ((phase == STORAGE_SCAN_DATA) || (phase == STORAGE_SCAN_COMMIT))
    {
        uint8_t zone = (scan_slot < 2u) ? (uint8_t)(scan_slot + 1u) : 4u;             /* 扫描区域索引。 */
        uint32_t offset = (scan_slot < 2u) ? 0u : ((uint32_t)scan_slot - 2u) * block; /* 日志块起点。 */
        uint32_t kind = (scan_slot < 2u) ? 1u : 2u;                                   /* 期望的记录类型。 */
        if (phase == STORAGE_SCAN_DATA)
        {
            if (transfer(1u, zone, offset, page, data_page) == 0u)
            {
                return;
            }
            if ((io_result == FAL_RESULT_SUCCESS) && (demo_storage_record_data_valid(data_page, kind) == 1u))
            {
                phase = STORAGE_SCAN_COMMIT;
            }
            else
            {
                scan_advance();
            }
        }
        else
        {
            if (transfer(1u, zone, offset + page, page, commit_page) == 0u)
            {
                return;
            }
            if ((io_result == FAL_RESULT_SUCCESS) && (demo_storage_record_committed(data_page, commit_page) == 1u))
            {
                uint32_t sequence = flash_integrity_u32_get(&data_page[8]); /* 已验证的记录序号。 */
                if (scan_slot < 2u)
                {
                    if (demo_storage_record_newer(sequence, views[d].sequence) == 1u)
                    {
                        views[d].sequence = sequence;
                        views[d].valid = 1u;
                        active_slot[d] = scan_slot;
                        demo_storage_record_decode(data_page, &recovered[d]);
                    }
                }
                else if (demo_storage_record_newer(sequence, log_sequence[d]) == 1u)
                {
                    log_sequence[d] = sequence;
                    views[d].log_sequence = sequence;
                    demo_storage_record_decode(data_page, &views[d].last_log);
                    log_slot[d] = (uint8_t)(scan_slot - 2u);
                }
            }
            scan_advance();
        }
        return;
    }
    if (phase == STORAGE_DECIDE)
    {
        decide();
        return;
    }
    if (phase == STORAGE_FINISH)
    {
        health_update(0u);
        health_update(1u);
        if (demo_storage_exchange_finish(request.id, outcome,
                                         (restore == 1u) ? &recovered[d] : NULL, views) == 1u)
        {
            phase = STORAGE_IDLE;
            booting = 0u;
        }
        return;
    }
    if (phase == STORAGE_ERASE)
    {
        uint32_t offset = target_offset; /* 本次擦除的区域内起点。 */
        uint32_t length = block;         /* 默认只擦一个不活动槽位。 */
        if (request.command == DEMO_STORAGE_TEST)
        {
            offset = 0u;
            length = 2u * block;
        }
        if (transfer(3u, target_zone, offset, length, NULL) == 0u)
        {
            return;
        }
        phase = STORAGE_WRITE;
    }
    else if (phase == STORAGE_WRITE)
    {
        if (transfer(2u, target_zone, target_offset, page, data_page) == 0u)
        {
            return;
        }
        phase = STORAGE_VERIFY;
    }
    else if (phase == STORAGE_VERIFY)
    {
        if (transfer(1u, target_zone, target_offset, page, verify_page) == 0u)
        {
            return;
        }
        if ((io_result == FAL_RESULT_SUCCESS) && (memcmp(data_page, verify_page, page) != 0))
        {
            outcome = -21;
            phase = STORAGE_FINISH;
            return;
        }
        phase = STORAGE_COMMIT;
    }
    else if (phase == STORAGE_COMMIT)
    {
        if (transfer(2u, target_zone, target_offset + page, page, commit_page) == 0u)
        {
            return;
        }
        phase = STORAGE_VERIFY_COMMIT;
    }
    else if (phase == STORAGE_VERIFY_COMMIT)
    {
        if (transfer(1u, target_zone, target_offset + page, page, verify_page) == 0u)
        {
            return;
        }
        if ((io_result == FAL_RESULT_SUCCESS) && (memcmp(commit_page, verify_page, page) != 0))
        {
            outcome = -21;
        }
        if ((io_result == FAL_RESULT_SUCCESS) && (outcome == 0) && (request.command == DEMO_STORAGE_SAVE))
        {
            views[d].valid = 1u;
            views[d].sequence = next_sequence;
            active_slot[d] = (uint8_t)(target_zone - 1u);
        }
        phase = STORAGE_FINISH;
    }
    else
    {
        return;
    }
    if ((phase == STORAGE_FINISH) && (io_result == FAL_RESULT_SUCCESS) &&
        (outcome == 0) && (request.command == DEMO_STORAGE_LOG))
    {
        views[d].log_sequence = next_sequence;
        views[d].last_log = request.params;
    }
    if (io_result != FAL_RESULT_SUCCESS)
    {
        outcome = (int32_t)io_result;
        health_update(d);
        views[d].online = 0u; /* 操作失败后需显式 SCAN 或下一次保存/加载重新探测。 */
        phase = STORAGE_FINISH;
    }
}
