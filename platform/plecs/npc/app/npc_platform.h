// SPDX-License-Identifier: MIT
/**
 * @file    npc_platform.h
 * @brief   NPC runtime log lifecycle.
 * @details This file is part of the base digital power framework project.
 *          Windows PLECS host logging; call while the communication server is stopped.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * This file is licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_PLATFORM_H
#define NPC_PLATFORM_H
#include <stddef.h>
/** @param p_header CSV column names, including time_s. @return 1 if a unique trace file opens. */
int npc_trace_start(const char *p_header);
/** @param time_s Simulation timestamp. @param p_values Snapshot. @param count Number of values.
 *  @return 1 on success, 0 on I/O failure. Single simulation thread only. */
int npc_trace_write(double time_s, const float *p_values, size_t count);
/** @brief Flush and close the current trace. */
void npc_trace_stop(void);
/** @return true if npc_runtime.log beside the loaded DLL was opened for append. */
int npc_log_start(void);
/** @brief Close the runtime log after stopping communication threads. */
void npc_log_stop(void);
#endif
