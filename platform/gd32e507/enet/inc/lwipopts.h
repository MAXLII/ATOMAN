// SPDX-License-Identifier: MIT
/**
 * @file    lwipopts.h
 * @brief   GD32E507 standalone LwIP configuration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure LwIP 2.2.1 for a bare-metal polling environment
 *          - Bound the TCP/IP heap and packet pools for 128 KiB SRAM
 *          - Enable IPv4 ICMP, TCP, UDP, ARP, and hardware checksum offload
 *
 *          Design notes:
 *          - C11 compatible
 *          - LwIP uses one fixed internal heap
 *          - Network processing runs only from the background ENET service task
 *          - Hardware access is abstracted through the GD32E507 ENET BSP
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_TIMERS 1
#define LWIP_TIMERS_CUSTOM 0

#define MEM_ALIGNMENT 4
#define MEM_SIZE (12u * 1024u)
#define MEMP_NUM_PBUF 12
#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_PCB_LISTEN 2
#define MEMP_NUM_TCP_SEG 12
#define MEMP_NUM_SYS_TIMEOUT 8

#define PBUF_POOL_SIZE 12
#define PBUF_POOL_BUFSIZE 1536
#define IP_REASS_MAX_PBUFS 8

#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ARP 1
#define LWIP_ICMP 1
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_DHCP 0
#define LWIP_AUTOIP 0
#define LWIP_IGMP 0
#define LWIP_DNS 0

#define TCP_TTL 64
#define TCP_QUEUE_OOSEQ 0
#define TCP_MSS 1460
#define TCP_SND_BUF (2u * TCP_MSS)
#define TCP_SND_QUEUELEN 8
#define TCP_WND (2u * TCP_MSS)
#define UDP_TTL 64

#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_STATS 0
#define LWIP_PROVIDE_ERRNO 1
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK 0
#define LWIP_SINGLE_NETIF 1

#define CHECKSUM_BY_HARDWARE
#define CHECKSUM_GEN_IP 0
#define CHECKSUM_GEN_UDP 0
#define CHECKSUM_GEN_TCP 0
#define CHECKSUM_GEN_ICMP 0
#define CHECKSUM_CHECK_IP 0
#define CHECKSUM_CHECK_UDP 0
#define CHECKSUM_CHECK_TCP 0

#define LWIP_DEBUG 0

#endif /* LWIPOPTS_H */
