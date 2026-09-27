// SPDX-License-Identifier: MIT
/**
 * @file comm_route_cfg.h
 * @brief Shared addresses and loopback transport for the two CHB simulation DLLs.
 * @details Link 1 serves FRAME; link 2 connects the controller and grid nodes.
 */
#ifndef CHB_COMM_ROUTE_CFG_H
#define CHB_COMM_ROUTE_CFG_H

#define CHB_ROUTE_FRAME_ADDR    0x01u /**< FRAME protocol source address. */
#define CHB_ROUTE_CTRL_ADDR     0x02u /**< Controller protocol address. */
#define CHB_ROUTE_GRID_ADDR     0x03u /**< Grid-source protocol address. */
#define CHB_ROUTE_PEER_TCP_PORT 5006u /**< Private loopback port; controller listens. */

#endif /* CHB_COMM_ROUTE_CFG_H */
