// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_record.c
 * @brief   Encode and validate versioned demo parameter records and separate commit pages.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Encode and validate versioned demo parameter records and separate commit pages.
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
#include "demo_storage_record.h"
#include "flash_integrity.h"
#include <stddef.h>
#include <string.h>
uint8_t demo_storage_record_params_valid(const demo_storage_params_t *p_params)
{
    if (p_params == NULL) { return 0u; }
    return ((p_params->input_mv <= 100000u) && (p_params->gain_milli <= 10000u) &&
            (p_params->threshold_mv <= 1000000u)) ? 1u : 0u;
}
void demo_storage_record_decode(const uint8_t *p_data, demo_storage_params_t *p_params)
{
    if ((p_data == NULL) || (p_params == NULL)) { return; }
    p_params->input_mv = flash_integrity_u32_get(&p_data[16]);
    p_params->gain_milli = flash_integrity_u32_get(&p_data[20]);
    p_params->threshold_mv = flash_integrity_u32_get(&p_data[24]);
}
void demo_storage_record_encode(uint8_t *p_data, uint8_t *p_commit, uint32_t size,
    uint32_t kind, uint32_t sequence, const demo_storage_params_t *p_params)
{
    if ((p_data == NULL) || (p_commit == NULL) || (p_params == NULL) || (size < 32u)) { return; }
    (void)memset(p_data, 0xFF, size);
    (void)memset(p_commit, 0xFF, size);
    flash_integrity_u32_put(p_data, 0x31545344u); /* D ST1：固定版本的记录格式。 */
    flash_integrity_u32_put(&p_data[4], kind);
    flash_integrity_u32_put(&p_data[8], sequence);
    flash_integrity_u32_put(&p_data[12], 12u);
    flash_integrity_u32_put(&p_data[16], p_params->input_mv);
    flash_integrity_u32_put(&p_data[20], p_params->gain_milli);
    flash_integrity_u32_put(&p_data[24], p_params->threshold_mv);
    flash_integrity_u32_put(&p_data[28], flash_integrity_crc32(p_data, 28u));
    flash_integrity_u32_put(p_commit, 0x31544D43u); /* CMT1：仅最后写入独立的提交页。 */
    flash_integrity_u32_put(&p_commit[4], kind);
    flash_integrity_u32_put(&p_commit[8], sequence);
    flash_integrity_u32_put(&p_commit[12], flash_integrity_u32_get(&p_data[28]));
    flash_integrity_u32_put(&p_commit[16], flash_integrity_crc32(p_commit, 16u));
}
uint8_t demo_storage_record_data_valid(const uint8_t *p_data, uint32_t kind)
{
    demo_storage_params_t params = {0}; /* 解码后进行语义范围检查。 */
    if (p_data == NULL) { return 0u; }
    if ((flash_integrity_u32_get(p_data) != 0x31545344u) ||
        (flash_integrity_u32_get(&p_data[4]) != kind) ||
        (flash_integrity_u32_get(&p_data[8]) == 0u) ||
        (flash_integrity_u32_get(&p_data[12]) != 12u) ||
        (flash_integrity_u32_get(&p_data[28]) != flash_integrity_crc32(p_data, 28u))) { return 0u; }
    demo_storage_record_decode(p_data, &params);
    return demo_storage_record_params_valid(&params);
}
uint8_t demo_storage_record_committed(const uint8_t *p_data, const uint8_t *p_commit)
{
    if ((p_data == NULL) || (p_commit == NULL)) { return 0u; }
    return ((flash_integrity_u32_get(p_commit) == 0x31544D43u) &&
            (flash_integrity_u32_get(&p_commit[4]) == flash_integrity_u32_get(&p_data[4])) &&
            (flash_integrity_u32_get(&p_commit[8]) == flash_integrity_u32_get(&p_data[8])) &&
            (flash_integrity_u32_get(&p_commit[12]) == flash_integrity_u32_get(&p_data[28])) &&
            (flash_integrity_u32_get(&p_commit[16]) == flash_integrity_crc32(p_commit, 16u))) ? 1u : 0u;
}
uint8_t demo_storage_record_newer(uint32_t candidate, uint32_t current)
{
    uint32_t distance = candidate - current; /* 无符号模差，支持序号回绕。 */
    return ((candidate != 0u) && ((current == 0u) || ((distance != 0u) && (distance < 0x80000000u)))) ? 1u : 0u;
}
