// SPDX-License-Identifier: MIT
/**
 * @file gd32e50x_enet.h
 * @brief Host-only MAC descriptor boundary for the production Ethernet port.
 * @details Models DMA ownership and address access; LwIP and port logic remain real.
 *          No MCU registers or hardware signal source are emulated for board acceptance.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef TEST_ENET_NONBLOCKING_H
#define TEST_ENET_NONBLOCKING_H
#include <stdint.h>
#define ENET_RXBUF_NUM                12
#define ENET_TXBUF_NUM                4
#define ENET_RXBUF_SIZE               1536
#define ENET_TXBUF_SIZE               1536
#define ENET_TDES0_DAV                UINT32_C(0x80000000)
#define RESET                         0u
#define ENET_MAC_ADDRESS0             0u
#define ENET_DMA_TX                   0u
#define ENET_DMA_RX                   1u
#define ENET_CHECKSUM_TCPUDPICMP_FULL 0u
#define TXDESC_BUFFER_1_ADDR          0u
#define RXDESC_BUFFER_1_ADDR          0u
#define RXDESC_FRAME_LENGTH           1u
typedef struct enet_descriptors_struct
{
    uint32_t status;
    uintptr_t buffer;
    uint32_t length;
} enet_descriptors_struct;
static inline void enet_mac_address_set(uint32_t slot, const uint8_t *p_address)
{
    (void)slot;
    (void)p_address;
}
static inline void enet_descriptors_chain_init(uint32_t direction)
{
    (void)direction;
}
static inline void enet_rx_desc_immediate_receive_complete_interrupt(enet_descriptors_struct *p_desc)
{
    (void)p_desc;
}
static inline void enet_transmit_checksum_config(enet_descriptors_struct *p_desc, uint32_t mode)
{
    (void)p_desc;
    (void)mode;
}
static inline void enet_enable(void)
{
}
static inline uintptr_t enet_desc_information_get(const enet_descriptors_struct *p_desc, uint32_t field)
{
    return field == RXDESC_FRAME_LENGTH ? p_desc->length : p_desc->buffer;
}
void test_enet_transmit(uint32_t length);
#define ENET_NOCOPY_FRAME_TRANSMIT(length) test_enet_transmit(length)
#define ENET_NOCOPY_FRAME_RECEIVE()        ((void)0)
#endif
