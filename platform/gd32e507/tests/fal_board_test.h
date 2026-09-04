// SPDX-License-Identifier: MIT
/**
 * @file    fal_board_test.h
 * @brief   Publish opt-in FAL hardware-test control and observations.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Expose a debugger-visible, fixed-width test report.
 *          - Separate test arming from normal firmware startup.
 *          Design notes:
 *          - C11 compatible; no dynamic allocation.
 *          - Commands are written only while idle; observations are polled by SWD.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * This file is licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef FAL_BOARD_TEST_H
#define FAL_BOARD_TEST_H
#include <stdint.h>
typedef struct fal_board_report
{
    uint32_t command;       /**< 0 idle, 1 full suite, 2 post-reset dual-device test. */
    uint32_t status;        /**< 0 unarmed, 1 running, 2 passed, 3 failed. */
    uint32_t case_id;       /**< Current S01..S09 case. */
    uint32_t iteration;     /**< Completed transactions in current case. */
    uint32_t passed[10];    /**< Per-case completed checks/transactions. */
    uint32_t failure;       /**< First failed assertion identifier; 0 on success. */
    int32_t expected;       /**< Expected value at failure. */
    int32_t actual;         /**< Observed value at failure. */
    uint32_t max_demo_us;   /**< Largest wall-clock Demo transaction duration. */
    uint32_t corrected;    /**< NAND corrected bits observed at completion. */
    uint32_t bad_blocks;   /**< NAND bad/quarantined block count. */
} fal_board_report_t;
extern volatile fal_board_report_t g_fal_board_report; /**< Task-owned report, debugger arms command once. */
#endif
