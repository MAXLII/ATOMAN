// SPDX-License-Identifier: MIT
/**
 * @file sim_tcp.h
 * @brief SECTION-registered TCP byte transport below platform BSP.
 * @details Static registration; serialized simulation callbacks; bounded storage.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */

#ifndef SIM_TCP_H
#define SIM_TCP_H
#include <stdarg.h>
#include "section.h"

typedef enum { SIM_TCP_SERVER = 0, SIM_TCP_CLIENT } SIM_TCP_ROLE_E;
typedef struct sim_tcp_status
{
    uint32_t connected;
    uint32_t session;
    uint32_t tx_dropped;
    uint32_t last_error;
} sim_tcp_status_t;
/* Opaque implementation state; the BSP owns only the registration handle. */
struct sim_tcp;
typedef struct sim_tcp_reg
{
    const char *p_name;
    SIM_TCP_ROLE_E role;
    const char *p_ip;
    uint16_t port;
    struct sim_tcp **p_handle;
    sim_tcp_status_t status;
} sim_tcp_reg_t;

#define REG_SIM_TCP(_handle, _name, _role, _ip, _port) \
    static struct sim_tcp *_handle = NULL;           \
    static sim_tcp_reg_t sim_tcp_reg_##_handle = {    \
        .p_name = (_name), .role = (_role),           \
        .p_ip = (_ip), .port = (_port),               \
        .p_handle = &(_handle), .status = {0},        \
    };                                               \
    REG_SECTION_FUNC(SECTION_SIM_TCP, sim_tcp_reg_##_handle)

/** Whole writes are copied to a bounded ring, or rejected and counted. */
void sim_tcp_tx(struct sim_tcp *p_tcp, const char *p_data, int len);
uint8_t sim_tcp_rx_get_byte(struct sim_tcp *p_tcp, uint8_t *p_data);
void sim_tcp_vprintf(struct sim_tcp *p_tcp, const char *p_format, va_list args);
/** Query a registered channel by name; unknown names return a disconnected zero status. */
const sim_tcp_status_t *sim_tcp_get_status(const char *p_name);
/** Close all endpoints and clear bound handles; safe before init and on repeated calls. */
void sim_tcp_stop(void);
#endif
