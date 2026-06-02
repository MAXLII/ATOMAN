// SPDX-License-Identifier: MIT
/**
 * @file    plecs_log_file_path.c
 * @brief   PLECS boost log path definition.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide the PLECS log output path for the boost simulation target
 *          - Keep the common PLECS logging module independent from project paths
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Included by plecs.c during PLECS simulation builds
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#define PLECS_LOG_FILE_PATH "\\\\wsl.localhost\\Ubuntu\\home\\zeus\\work\\base\\plecs\\boost\\plecs_log.txt"
