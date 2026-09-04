// SPDX-License-Identifier: MIT
/**
 * @file    demo_fal.h
 * @brief   Register task-context Flash demo control and data-pool observation.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Register task-context Flash demo control and data-pool observation.
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
#ifndef DEMO_FAL_H
#define DEMO_FAL_H
/* This demo is selected only by targets with the Flash Interface implementation. */
#endif
