// SPDX-License-Identifier: MIT
/**
 * @file    bsp_enet.c
 * @brief   GD32E507Z-EVAL Ethernet BSP implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the GD32E507 RMII pins for the onboard DP83848 PHY
 *          - Reset and initialize the ENET MAC/DMA with hardware checksums
 *          - Read the PHY link state and expose pending receive frames
 *          - Build compact BSP-owned responses for FRAME UDP device discovery
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The board supplies RMII_REF_CLK from its onboard 50 MHz oscillator
 *          - Hardware access uses the GD32E50x standard peripheral library
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

#include "bsp_enet.h"

#include "enet_config.h"
#include "gd32e50x.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include <string.h>

#define BSP_ENET_DISCOVERY_DEVICE_NAME "GD32E507Z-EVAL" /* Board name shown by FRAME. */
#define BSP_ENET_DISCOVERY_FIRMWARE_VERSION "1.0.0" /* Firmware identity reported to FRAME. */
#define BSP_ENET_DISCOVERY_PROTOCOL_VERSION "1" /* Discovery response schema version. */
#define BSP_ENET_DISCOVERY_REQUEST_LENGTH 17u /* Exact byte length of "FRAME_DISCOVER_V1". */
#define BSP_ENET_DISCOVERY_RESPONSE_MAX_LENGTH 192u /* Maximum ASCII discovery response length. */

volatile uint32_t g_bsp_enet_init_attempt_count = 0u;  /* Number of MAC/PHY initialization attempts. */
volatile uint32_t g_bsp_enet_init_error_count = 0u;    /* Number of failed MAC/PHY initialization attempts. */
volatile uint32_t g_bsp_enet_phy_read_error_count = 0u; /* Number of failed PHY status reads. */

static void gpio_config(void);
static uint8_t response_append_text(uint8_t *p_response,
                                    uint16_t response_capacity,
                                    uint16_t *p_response_length,
                                    const char *p_text);
static uint8_t response_append_u16(uint8_t *p_response,
                                   uint16_t response_capacity,
                                   uint16_t *p_response_length,
                                   uint16_t value);
static uint8_t response_append_hex_u8(uint8_t *p_response,
                                      uint16_t response_capacity,
                                      uint16_t *p_response_length,
                                      uint8_t value);
static uint16_t discovery_response_build(const uint8_t *p_request,
                                         uint16_t request_length,
                                         uint8_t *p_response,
                                         uint16_t response_capacity);

static uint8_t response_append_text(uint8_t *p_response,
                                    uint16_t response_capacity,
                                    uint16_t *p_response_length,
                                    const char *p_text)
{
    size_t text_length = 0u; /* Number of source bytes appended without a terminating NUL. */
    uint16_t available_length = 0u; /* Remaining caller-owned response capacity. */

    if ((p_response == NULL) || /* The caller did not provide response storage. */
        (p_response_length == NULL) || /* The current output position is unavailable. */
        (p_text == NULL) || /* The response fragment is unavailable. */
        (*p_response_length > response_capacity)) /* The caller supplied an invalid current length. */
    {
        return 0u;
    }

    text_length = strlen(p_text);
    available_length = (uint16_t)(response_capacity - *p_response_length);
    if (text_length > (size_t)available_length)
    {
        return 0u;
    }

    (void)memcpy(&p_response[*p_response_length], p_text, text_length);
    *p_response_length = (uint16_t)((size_t)*p_response_length + text_length);
    return 1u;
}

static uint8_t response_append_u16(uint8_t *p_response,
                                   uint16_t response_capacity,
                                   uint16_t *p_response_length,
                                   uint16_t value)
{
    uint8_t digits[5] = {0u}; /* Reversed decimal digits for the complete uint16_t range. */
    uint16_t digit_count = 0u; /* Number of meaningful entries in digits. */
    uint16_t remaining_value = value; /* Value portion not yet converted to decimal. */
    uint16_t index = 0u; /* Reverse-copy position in digits. */

    do
    {
        digits[digit_count] = (uint8_t)('0' + (remaining_value % 10u));
        digit_count++;
        remaining_value = (uint16_t)(remaining_value / 10u);
    } while (remaining_value > 0u);

    if ((p_response == NULL) || /* The caller did not provide response storage. */
        (p_response_length == NULL) || /* The current output position is unavailable. */
        (*p_response_length > response_capacity) || /* The current length is outside the buffer. */
        (digit_count > (uint16_t)(response_capacity - *p_response_length))) /* Decimal text would overflow. */
    {
        return 0u;
    }

    for (index = 0u; index < digit_count; index++)
    {
        p_response[*p_response_length] = digits[(uint16_t)(digit_count - index - 1u)];
        *p_response_length = (uint16_t)(*p_response_length + 1u);
    }
    return 1u;
}

static uint8_t response_append_hex_u8(uint8_t *p_response,
                                      uint16_t response_capacity,
                                      uint16_t *p_response_length,
                                      uint8_t value)
{
    static const uint8_t hex_digits[] = "0123456789ABCDEF"; /* Upper-case MAC address alphabet. */

    if ((p_response == NULL) || /* The caller did not provide response storage. */
        (p_response_length == NULL) || /* The current output position is unavailable. */
        (*p_response_length > response_capacity) || /* The current length is outside the buffer. */
        ((uint16_t)(response_capacity - *p_response_length) < 2u)) /* A MAC octet needs 2 characters. */
    {
        return 0u;
    }

    p_response[*p_response_length] = hex_digits[(value >> 4u) & 0x0Fu];
    *p_response_length = (uint16_t)(*p_response_length + 1u);
    p_response[*p_response_length] = hex_digits[value & 0x0Fu];
    *p_response_length = (uint16_t)(*p_response_length + 1u);
    return 1u;
}

static void gpio_config(void)
{
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_ethernet_phy_select(GPIO_ENET_PHY_RMII);

    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_7);
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5);
}

uint8_t bsp_enet_init(void)
{
    ErrStatus reset_status = ERROR; /* ENET DMA software-reset result. */
    ErrStatus init_status = ERROR;  /* PHY negotiation and MAC initialization result. */

    g_bsp_enet_init_attempt_count++;
    gpio_config();

    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);

    enet_deinit();
    reset_status = enet_software_reset();
    if (reset_status != SUCCESS)
    {
        g_bsp_enet_init_error_count++;
        return 0u;
    }

    init_status = enet_init(ENET_AUTO_NEGOTIATION,
                            ENET_AUTOCHECKSUM_DROP_FAILFRAMES,
                            ENET_BROADCAST_FRAMES_PASS);
    if (init_status != SUCCESS)
    {
        g_bsp_enet_init_error_count++;
        return 0u;
    }

    return 1u;
}

uint8_t bsp_enet_link_is_up(void)
{
    uint16_t phy_status = 0u;       /* DP83848 basic status register value. */
    ErrStatus read_status = ERROR;  /* MDIO read result. */

    read_status = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_status);
    if (read_status == SUCCESS)
    {
        read_status = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_status);
    }

    if (read_status != SUCCESS)
    {
        g_bsp_enet_phy_read_error_count++;
        return 0u;
    }

    return ((phy_status & PHY_LINKED_STATUS) != 0u) ? 1u : 0u;
}

uint8_t bsp_enet_rx_frame_pending(void)
{
    return (enet_rxframe_size_get() != 0u) ? 1u : 0u;
}

static uint16_t discovery_response_build(const uint8_t *p_request,
                                         uint16_t request_length,
                                         uint8_t *p_response,
                                         uint16_t response_capacity)
{
    static const uint8_t discovery_request[] = "FRAME_DISCOVER_V1"; /* Exact FRAME host discovery probe. */
    uint16_t response_length = 0u; /* Number of valid ASCII response bytes produced for the caller. */

    if ((p_request == NULL) || /* The UDP adapter did not provide request bytes. */
        (p_response == NULL) || /* The UDP adapter did not provide response storage. */
        (request_length != BSP_ENET_DISCOVERY_REQUEST_LENGTH) || /* Only the complete probe is accepted. */
        (memcmp(p_request, discovery_request, BSP_ENET_DISCOVERY_REQUEST_LENGTH) != 0)) /* Reject UDP Echo traffic. */
    {
        return 0u;
    }

    if (response_append_text(p_response,
                             response_capacity,
                             &response_length,
                             "FRAME_DEVICE_V1;name=" BSP_ENET_DISCOVERY_DEVICE_NAME ";ip=") == 0u)
    {
        return 0u;
    }
    if (response_append_u16(p_response,
                            response_capacity,
                            &response_length,
                            ENET_CONFIG_IP_ADDR0) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ".") == 0u)
    {
        return 0u;
    }
    if (response_append_u16(p_response,
                            response_capacity,
                            &response_length,
                            ENET_CONFIG_IP_ADDR1) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ".") == 0u)
    {
        return 0u;
    }
    if (response_append_u16(p_response,
                            response_capacity,
                            &response_length,
                            ENET_CONFIG_IP_ADDR2) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ".") == 0u)
    {
        return 0u;
    }
    if (response_append_u16(p_response,
                            response_capacity,
                            &response_length,
                            ENET_CONFIG_IP_ADDR3) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ";tcp_port=") == 0u)
    {
        return 0u;
    }
    if (response_append_u16(p_response,
                            response_capacity,
                            &response_length,
                            ENET_CONFIG_TCP_FRAME_PORT) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ";mac=") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR0) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ":") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR1) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ":") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR2) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ":") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR3) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ":") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR4) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response, response_capacity, &response_length, ":") == 0u)
    {
        return 0u;
    }
    if (response_append_hex_u8(p_response, response_capacity, &response_length, BOARD_MAC_ADDR5) == 0u)
    {
        return 0u;
    }
    if (response_append_text(p_response,
                             response_capacity,
                             &response_length,
                             ";fw_version=" BSP_ENET_DISCOVERY_FIRMWARE_VERSION
                             ";protocol_version=" BSP_ENET_DISCOVERY_PROTOCOL_VERSION) == 0u)
    {
        return 0u;
    }

    return response_length;
}

bsp_enet_discovery_result_t bsp_enet_discovery_udp_process(struct udp_pcb *p_endpoint,
                                                            const struct pbuf *p_packet,
                                                            const struct ip4_addr *p_remote_address,
                                                            uint16_t remote_port)
{
    uint8_t request[BSP_ENET_DISCOVERY_REQUEST_LENGTH] = {0u}; /* Contiguous discovery probe from LwIP. */
    uint8_t response[BSP_ENET_DISCOVERY_RESPONSE_MAX_LENGTH] = {0u}; /* ASCII device identity response. */
    uint16_t response_length = 0u; /* Number of valid discovery response bytes. */
    u16_t copied_length = 0u; /* Request bytes copied from a possibly chained receive packet. */
    struct pbuf *p_response_packet = NULL; /* Temporary LwIP packet carrying the discovery response. */
    err_t send_status = ERR_OK; /* Response copy or UDP transmission result. */

    if ((p_endpoint == NULL) || /* The UDP service did not provide its bound endpoint. */
        (p_packet == NULL) || /* The UDP service did not provide a datagram. */
        (p_remote_address == NULL) || /* The response destination is unavailable. */
        (p_packet->tot_len != BSP_ENET_DISCOVERY_REQUEST_LENGTH)) /* Unrelated UDP traffic remains Echo data. */
    {
        return BSP_ENET_DISCOVERY_NOT_HANDLED_E;
    }

    copied_length = pbuf_copy_partial(p_packet,
                                      request,
                                      BSP_ENET_DISCOVERY_REQUEST_LENGTH,
                                      0u);
    if (copied_length != BSP_ENET_DISCOVERY_REQUEST_LENGTH)
    {
        return BSP_ENET_DISCOVERY_NOT_HANDLED_E;
    }

    response_length = discovery_response_build(request,
                                               BSP_ENET_DISCOVERY_REQUEST_LENGTH,
                                               response,
                                               BSP_ENET_DISCOVERY_RESPONSE_MAX_LENGTH);
    if (response_length == 0u)
    {
        return BSP_ENET_DISCOVERY_NOT_HANDLED_E;
    }

    p_response_packet = pbuf_alloc(PBUF_RAW, response_length, PBUF_POOL);
    if (p_response_packet == NULL)
    {
        return BSP_ENET_DISCOVERY_RESPONSE_ERROR_E;
    }

    send_status = pbuf_take(p_response_packet, response, response_length);
    if (send_status == ERR_OK)
    {
        send_status = udp_sendto(p_endpoint,
                                 p_response_packet,
                                 p_remote_address,
                                 remote_port);
    }
    pbuf_free(p_response_packet);

    return (send_status == ERR_OK) ? BSP_ENET_DISCOVERY_RESPONSE_SENT_E
                                   : BSP_ENET_DISCOVERY_RESPONSE_ERROR_E;
}
