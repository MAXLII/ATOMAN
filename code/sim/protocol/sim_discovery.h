// SPDX-License-Identifier: MIT
/**
 * @file sim_discovery.h
 * @brief FRAME UDP discovery registrations, separate from TCP transport.
 * @details Static registration; serialized simulation callbacks; bounded storage.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */

#ifndef SIM_DISCOVERY_H
#define SIM_DISCOVERY_H
#include "section.h"
typedef struct sim_discovery_reg
{
    const char *p_name;
    uint32_t node_addr;
    uint16_t tcp_port;
    uint16_t udp_port;
} sim_discovery_reg_t;
#define REG_SIM_DISCOVERY(_id, _name, _addr, _port, _udp_port) \
    static sim_discovery_reg_t sim_discovery_##_id = { \
        .p_name = (_name), .node_addr = (_addr), .tcp_port = (_port), .udp_port = (_udp_port), \
    }; \
    REG_SECTION_FUNC(SECTION_SIM_DISCOVERY, sim_discovery_##_id)
void sim_discovery_stop(void);
#endif
