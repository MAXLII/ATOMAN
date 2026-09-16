// SPDX-License-Identifier: MIT
/**
 * @file comm_link.h
 * @brief node02 TCP link identifiers and protocol addresses.
 * @details Static node configuration owned by this layer.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */
#ifndef COMM_LINK_H
#define COMM_LINK_H

#define COMM_LINK_DEVICE_ADDR 0x02u
#define COMM_LINK_PEER_ADDR 0x03u
#define COMM_LINK_PC_ADDR 0x01u

typedef enum
{
    TCP_DBG_LINK = 1,
    TCP_ISO_LINK,
} TCP_LINK_E;

#endif
