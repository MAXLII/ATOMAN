// SPDX-License-Identifier: MIT
/**
 * @file demo_comm.h
 * @brief Demo protocol callbacks consumed by the platform Link bridge.
 * @details Entity: byte and session callbacks. Prior: SECTION services initialized.
 *          Time: serialized Link task callbacks; no BSP or SDK dependencies.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef DEMO_COMM_H
#define DEMO_COMM_H
#include "section.h"
void demo_comm_usb_receive(uint8_t data, DEC_MY_PRINTF, void *p_context);
void demo_comm_wifi_receive(uint8_t data, DEC_MY_PRINTF, void *p_context);
/** Forget protocol history before accepting the first byte of a new TCP session. */
void demo_comm_wifi_session_reset(void);
#endif
