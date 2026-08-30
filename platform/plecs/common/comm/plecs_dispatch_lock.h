// SPDX-License-Identifier: MIT
/**
 * @file    plecs_dispatch_lock.h
 * @brief   Shared PLECS protocol-dispatch lock interface.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Declare the DLL-wide lock used by simulation and transport callbacks
 *          - Define explicit lifecycle operations for Windows synchronization state
 *          - Keep transport modules independent from the owning PLECS application
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The lock is process-thread safe but remains private to one loaded DLL
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
#ifndef PLECS_DISPATCH_LOCK_H
#define PLECS_DISPATCH_LOCK_H

void plecs_dispatch_lock_start(void);
void plecs_dispatch_lock_stop(void);
void plecs_dispatch_lock_enter(void);
void plecs_dispatch_lock_exit(void);

#endif /* PLECS_DISPATCH_LOCK_H */
