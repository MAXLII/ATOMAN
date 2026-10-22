// SPDX-License-Identifier: MIT
/**
 * @file app_main.h
 * @brief ESP-IDF firmware entry declaration.
 * @details Part of base. The implementation is in this platform project's src/app_main.c.
 *          SDK startup invokes it in task context.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef APP_MAIN_H
#define APP_MAIN_H
/** @brief Initialize this platform and start its application dispatcher. */
void app_main(void);
#endif /* APP_MAIN_H */
