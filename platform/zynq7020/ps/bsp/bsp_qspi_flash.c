// SPDX-License-Identifier: MIT
/**
 * @file    bsp_qspi_flash.c
 * @brief   Zynq-7020 PS QSPI NOR physical flash implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind XQspiPs polled transfers to the FAL physical device contract
 *          - Enforce 24-bit address, page, sector, and transfer boundaries
 *          - Poll one status-register sample per FAL process step after program or erase
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Calls are serialized by one FAL instance
 *          - The implementation targets the board's single 16 MiB QSPI NOR device
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_qspi_flash.h"

#include "xparameters.h"
#include "xqspips.h"
#include "xstatus.h"

#include <stddef.h>
#include <string.h>

#define QSPI_CMD_READ 0x03u
#define QSPI_CMD_PAGE_PROGRAM 0x02u
#define QSPI_CMD_READ_STATUS 0x05u
#define QSPI_CMD_WRITE_ENABLE 0x06u
#define QSPI_CMD_SECTOR_ERASE 0xD8u
#define QSPI_CMD_READ_ID 0x9Fu
#define QSPI_COMMAND_SIZE 4u
#define QSPI_STATUS_WIP 0x01u

typedef struct
{
    XQspiPs instance;
    uint8_t transfer_buffer[BSP_QSPI_FLASH_MAX_READ + QSPI_COMMAND_SIZE];
    uint8_t receive_buffer[BSP_QSPI_FLASH_MAX_READ + QSPI_COMMAND_SIZE];
    uint8_t operation_pending;
    uint8_t initialized;
} bsp_qspi_flash_t;

static bsp_qspi_flash_t s_qspi_flash;

static bsp_qspi_flash_t *context_resolve(void *p_context)
{
    return (p_context == NULL) ? &s_qspi_flash : (bsp_qspi_flash_t *)p_context;
}

static fal_result_t transfer(bsp_qspi_flash_t *p_flash,
                             uint8_t *p_send,
                             uint8_t *p_receive,
                             uint32_t length)
{
    return (XQspiPs_PolledTransfer(&p_flash->instance, p_send, p_receive, length) == XST_SUCCESS)
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t write_enable(bsp_qspi_flash_t *p_flash)
{
    uint8_t command = QSPI_CMD_WRITE_ENABLE;

    return transfer(p_flash, &command, NULL, 1u);
}

static void command_address_set(uint8_t *p_command, uint8_t command, uint32_t address)
{
    p_command[0] = command;
    p_command[1] = (uint8_t)((address >> 16u) & 0xFFu);
    p_command[2] = (uint8_t)((address >> 8u) & 0xFFu);
    p_command[3] = (uint8_t)(address & 0xFFu);
}

fal_result_t bsp_qspi_flash_init(void *p_context)
{
    bsp_qspi_flash_t *p_flash = context_resolve(p_context);
    XQspiPs_Config *p_config = NULL;
    uint8_t id_command[4] = {QSPI_CMD_READ_ID, 0u, 0u, 0u};
    uint8_t id_response[4] = {0};

    p_config = XQspiPs_LookupConfig(XPAR_XQSPIPS_0_DEVICE_ID);
    if ((p_config == NULL) ||
        (XQspiPs_CfgInitialize(&p_flash->instance, p_config, p_config->BaseAddress) != XST_SUCCESS) ||
        (XQspiPs_SetOptions(&p_flash->instance,
                            XQSPIPS_MANUAL_START_OPTION |
                                XQSPIPS_FORCE_SSELECT_OPTION |
                                XQSPIPS_HOLD_B_DRIVE_OPTION) != XST_SUCCESS) ||
        (XQspiPs_SetClkPrescaler(&p_flash->instance, XQSPIPS_CLK_PRESCALE_8) != XST_SUCCESS) ||
        (XQspiPs_SetSlaveSelect(&p_flash->instance) != XST_SUCCESS) ||
        (transfer(p_flash, id_command, id_response, (uint32_t)sizeof(id_command)) != FAL_RESULT_SUCCESS))
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    if ((id_response[1] == 0u) || (id_response[1] == 0xFFu) || (id_response[3] != 0x18u))
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    p_flash->operation_pending = 0u;
    p_flash->initialized = 1u;
    return FAL_RESULT_SUCCESS;
}

fal_device_state_t bsp_qspi_flash_state_get(void *p_context)
{
    bsp_qspi_flash_t *p_flash = context_resolve(p_context);
    uint8_t status_command[2] = {QSPI_CMD_READ_STATUS, 0u};
    uint8_t status_response[2] = {0};

    if (p_flash->initialized == 0u)
    {
        return FAL_DEVICE_STATE_ERROR;
    }
    if (p_flash->operation_pending == 0u)
    {
        return FAL_DEVICE_STATE_READY;
    }
    if (transfer(p_flash, status_command, status_response,
                 (uint32_t)sizeof(status_command)) != FAL_RESULT_SUCCESS)
    {
        p_flash->operation_pending = 0u;
        return FAL_DEVICE_STATE_ERROR;
    }
    if ((status_response[1] & QSPI_STATUS_WIP) != 0u)
    {
        return FAL_DEVICE_STATE_BUSY;
    }
    p_flash->operation_pending = 0u;
    return FAL_DEVICE_STATE_READY;
}

fal_result_t bsp_qspi_flash_read(void *p_context,
                                 uint32_t address,
                                 uint32_t length,
                                 uint8_t *p_data)
{
    bsp_qspi_flash_t *p_flash = context_resolve(p_context);

    if ((p_data == NULL) || (length == 0u) ||
        (length > BSP_QSPI_FLASH_MAX_READ) ||
        (address > BSP_QSPI_FLASH_CAPACITY) ||
        (length > (BSP_QSPI_FLASH_CAPACITY - address)) ||
        (p_flash->operation_pending != 0u))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    (void)memset(p_flash->transfer_buffer, 0, length + QSPI_COMMAND_SIZE);
    command_address_set(p_flash->transfer_buffer, QSPI_CMD_READ, address);
    if (transfer(p_flash,
                 p_flash->transfer_buffer,
                 p_flash->receive_buffer,
                 length + QSPI_COMMAND_SIZE) != FAL_RESULT_SUCCESS)
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    (void)memcpy(p_data, &p_flash->receive_buffer[QSPI_COMMAND_SIZE], length);
    return FAL_RESULT_SUCCESS;
}

fal_result_t bsp_qspi_flash_program(void *p_context,
                                    uint32_t address,
                                    uint32_t length,
                                    const uint8_t *p_data)
{
    bsp_qspi_flash_t *p_flash = context_resolve(p_context);

    if ((p_data == NULL) || (length == 0u) ||
        (length > BSP_QSPI_FLASH_PAGE_SIZE) ||
        ((address / BSP_QSPI_FLASH_PAGE_SIZE) !=
         ((address + length - 1u) / BSP_QSPI_FLASH_PAGE_SIZE)) ||
        (address > BSP_QSPI_FLASH_CAPACITY) ||
        (length > (BSP_QSPI_FLASH_CAPACITY - address)) ||
        (p_flash->operation_pending != 0u))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if (write_enable(p_flash) != FAL_RESULT_SUCCESS)
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    command_address_set(p_flash->transfer_buffer, QSPI_CMD_PAGE_PROGRAM, address);
    (void)memcpy(&p_flash->transfer_buffer[QSPI_COMMAND_SIZE], p_data, length);
    if (transfer(p_flash, p_flash->transfer_buffer, NULL,
                 length + QSPI_COMMAND_SIZE) != FAL_RESULT_SUCCESS)
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    p_flash->operation_pending = 1u;
    return FAL_RESULT_SUCCESS;
}

fal_result_t bsp_qspi_flash_erase(void *p_context, uint32_t address, uint32_t length)
{
    bsp_qspi_flash_t *p_flash = context_resolve(p_context);

    if ((length != BSP_QSPI_FLASH_ERASE_SIZE) ||
        ((address % BSP_QSPI_FLASH_ERASE_SIZE) != 0u) ||
        (address > BSP_QSPI_FLASH_CAPACITY) ||
        (length > (BSP_QSPI_FLASH_CAPACITY - address)) ||
        (p_flash->operation_pending != 0u))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if (write_enable(p_flash) != FAL_RESULT_SUCCESS)
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    command_address_set(p_flash->transfer_buffer, QSPI_CMD_SECTOR_ERASE, address);
    if (transfer(p_flash, p_flash->transfer_buffer, NULL, QSPI_COMMAND_SIZE) != FAL_RESULT_SUCCESS)
    {
        return FAL_RESULT_DRIVER_ERROR;
    }
    p_flash->operation_pending = 1u;
    return FAL_RESULT_SUCCESS;
}

fal_result_t bsp_qspi_flash_sync(void *p_context)
{
    const fal_device_state_t state = bsp_qspi_flash_state_get(p_context);

    return (state == FAL_DEVICE_STATE_READY)
               ? FAL_RESULT_SUCCESS
               : ((state == FAL_DEVICE_STATE_BUSY) ? FAL_RESULT_BUSY : FAL_RESULT_DRIVER_ERROR);
}

void *bsp_qspi_flash_context_get(void)
{
    return &s_qspi_flash;
}
