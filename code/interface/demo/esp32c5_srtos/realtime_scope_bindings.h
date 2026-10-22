// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_bindings.h
 * @brief Bind the scope service using application-owned sample storage.
 * @details Entity: binding API. Prior: initialized BSP. Time: initialization or task context.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef REALTIME_SCOPE_BINDINGS_H
#define REALTIME_SCOPE_BINDINGS_H
#include <stdbool.h>
#include <stdint.h>
/** @param p_queue Application storage, retained for the entire service lifetime.
 * @param capacity Storage size in bytes. @return Whether the service was bound. */
bool realtime_scope_bindings_bind(uint8_t *p_queue, uint32_t capacity);
#endif
