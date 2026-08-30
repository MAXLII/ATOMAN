// SPDX-License-Identifier: MIT
/**
 * @file    plecs_dispatch_lock.c
 * @brief   Shared PLECS protocol-dispatch lock implementation.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Serialize PLECS simulation callbacks with transport protocol dispatch
 *          - Own the Windows critical section used by every link in one DLL
 *          - Provide deterministic initialization and teardown for repeated simulations
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - All transport workers must stop before the lock is destroyed
 *          - Hardware access is not used by this simulation module
 *
 * @author  Max.Li
 * @date    2026-08-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "plecs_dispatch_lock.h"

#include <stdint.h>
#include <windows.h>

static CRITICAL_SECTION dispatch_lock; /**< Serializes all protocol and simulation callbacks in one DLL. */
static uint8_t dispatch_lock_ready = 0u; /**< Whether dispatch_lock has been initialized. */

void plecs_dispatch_lock_start(void)
{
    if (dispatch_lock_ready == 0u)
    {
        InitializeCriticalSection(&dispatch_lock);
        dispatch_lock_ready = 1u;
    }
}

void plecs_dispatch_lock_stop(void)
{
    if (dispatch_lock_ready == 1u)
    {
        DeleteCriticalSection(&dispatch_lock);
        dispatch_lock_ready = 0u;
    }
}

void plecs_dispatch_lock_enter(void)
{
    if (dispatch_lock_ready == 1u)
    {
        EnterCriticalSection(&dispatch_lock);
    }
}

void plecs_dispatch_lock_exit(void)
{
    if (dispatch_lock_ready == 1u)
    {
        LeaveCriticalSection(&dispatch_lock);
    }
}
