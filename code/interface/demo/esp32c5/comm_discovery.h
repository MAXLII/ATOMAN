// SPDX-License-Identifier: MIT
/**
 * @file comm_discovery.h
 * @brief Demo Interface entry points for device discovery and UDP echo.
 * @details Part of base. Called by the platform main task above BSP byte transport.
 *          Firmware identity is supplied by the platform without exposing SDK types.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef COMM_DISCOVERY_H
#define COMM_DISCOVERY_H
/** @brief Supply a firmware version string whose storage remains valid after return. */
void comm_discovery_init(const char *p_version);
/** @brief Service at most one pending datagram from the main task. */
void comm_discovery_poll(void);
#endif /* COMM_DISCOVERY_H */
