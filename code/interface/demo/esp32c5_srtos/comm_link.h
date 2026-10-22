// SPDX-License-Identifier: MIT
/**
 * @file comm_link.h
 * @brief Demo Interface link identifiers for the ESP32-C5 build.
 * @details Part of base. The debug serial name maps to USB Serial/JTAG on this board.
 *          Only the two implemented public control links are declared.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef COMM_LINK_H
#define COMM_LINK_H
typedef enum
{
    USART_DBG_LINK = 0, /**< USB Serial/JTAG debug stream. */
    WIFI_DBG_LINK  = 1  /**< Public Wi-Fi TCP control stream. */
} DEMO_LINK_E;
#endif /* COMM_LINK_H */
