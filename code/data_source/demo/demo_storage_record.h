// SPDX-License-Identifier: MIT
/**
 * @file    demo_storage_record.h
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
#ifndef DEMO_STORAGE_RECORD_H
#define DEMO_STORAGE_RECORD_H
#include "demo_storage_pool.h"
/** @param p_params 参数快照。 @return 1 合法，0 非法。 */
uint8_t demo_storage_record_params_valid(const demo_storage_params_t *p_params);
/** @param p_data 数据页。 @param p_commit 提交页。 @param size 页字节数。
 * @param kind 1 参数、2 日志、3 测试。 @param sequence 非零序号。 @param p_params 冻结的参数。 */
void demo_storage_record_encode(uint8_t *p_data, uint8_t *p_commit, uint32_t size,
    uint32_t kind, uint32_t sequence, const demo_storage_params_t *p_params);
/** @param p_data 数据页。 @param kind 预期类型。 @return 1 数据页完整，0 非法。 */
uint8_t demo_storage_record_data_valid(const uint8_t *p_data, uint32_t kind);
/** @param p_data 数据页。 @param p_commit 提交页。 @return 1 提交完整且匹配，0 非法。 */
uint8_t demo_storage_record_committed(const uint8_t *p_data, const uint8_t *p_commit);
/** @param p_data 已验证的数据页。 @param p_params 解码输出。 */
void demo_storage_record_decode(const uint8_t *p_data, demo_storage_params_t *p_params);
/** @param candidate 候选序号。 @param current 当前序号。 @return 1 更新，0 否；按模 2^32 比较。 */
uint8_t demo_storage_record_newer(uint32_t candidate, uint32_t current);
#endif
