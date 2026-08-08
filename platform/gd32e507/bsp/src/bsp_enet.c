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

#include "gd32e50x.h"

volatile uint32_t g_bsp_enet_init_attempt_count = 0u;  /* Number of MAC/PHY initialization attempts. */
volatile uint32_t g_bsp_enet_init_error_count = 0u;    /* Number of failed MAC/PHY initialization attempts. */
volatile uint32_t g_bsp_enet_phy_read_error_count = 0u; /* Number of failed PHY status reads. */

static void gpio_config(void);

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
