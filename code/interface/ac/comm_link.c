#include "comm_link.h"
#include "comm_addr.h"
#include "section.h"
#include "comm.h"
#include "shell.h"
#include "bsp_usart.h"

#include <stdint.h>

#if (COMM_LINK_ENABLE_CAN == 1)
#include "bsp_can.h"
#endif

#ifndef AC_USART_DBG_COMM_PAYLOAD_SIZE
#define AC_USART_DBG_COMM_PAYLOAD_SIZE 128U
#endif

#ifndef AC_USART_ISO_COMM_PAYLOAD_SIZE
#define AC_USART_ISO_COMM_PAYLOAD_SIZE 128U
#endif

#ifndef AC_USART_PL_COMM_PAYLOAD_SIZE
#define AC_USART_PL_COMM_PAYLOAD_SIZE 512U
#endif

#ifndef AC_CAN_DBG_COMM_PAYLOAD_SIZE
#define AC_CAN_DBG_COMM_PAYLOAD_SIZE 128U
#endif

static volatile uint8_t usart1_suspended = 0U; /* USART1 background link suspension state. */

void comm_link_usart1_suspend(uint8_t suspended)
{
    usart1_suspended = (suspended != 0U) ? 1U : 0U;
    __DMB();
}

static void usart_dbg_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    bsp_usart_dbg_tx(ptr, len);
}

static void usart_iso_tx_by_dma_cb(char *ptr, int len)
{
#if (COMM_LINK_ENABLE_ISO == 1)
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    bsp_usart_iso_tx(ptr, len);
#else
    (void)ptr;
    (void)len;
#endif
}

static void usart_pl_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) ||
        (len <= 0) ||
        (usart1_suspended == 1U))
    {
        return;
    }

    (void)bsp_usart_pl_tx_dma((const uint8_t *)ptr, (uint32_t)len);
}

static uint8_t usart_pl_rx_get_byte_cb(uint8_t *p_data)
{
    if (usart1_suspended == 1U)
    {
        return 0U;
    }

    return bsp_usart_pl_rx_get_byte(p_data);
}

#if (COMM_LINK_ENABLE_CAN == 1)
static void can_dbg_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    bsp_can_dbg_tx(ptr, len);
}
#endif

static uint8_t comm_link_empty_rx_get_byte(uint8_t *p_data)
{
    (void)p_data;
    return 0u;
}

static section_link_tx_func_t s_usart_dbg_tx_func = {
    .my_printf = bsp_usart_dbg_printf,
    .tx_by_dma = usart_dbg_tx_by_dma_cb,
};

static section_link_tx_func_t s_usart_iso_tx_func = {
#if (COMM_LINK_ENABLE_ISO == 1)
    .my_printf = bsp_usart_iso_printf,
#else
    .my_printf = NULL,
#endif
    .tx_by_dma = usart_iso_tx_by_dma_cb,
};

static section_link_tx_func_t s_usart_pl_tx_func = {
    .my_printf = bsp_usart_pl_printf,
    .tx_by_dma = usart_pl_tx_by_dma_cb,
};

#if (COMM_LINK_ENABLE_CAN == 1)
static section_link_tx_func_t s_can_dbg_tx_func = {
    .my_printf = bsp_can_dbg_printf,
    .tx_by_dma = can_dbg_tx_by_dma_cb,
};
#endif

static section_link_tx_func_t s_usart_null_tx_func = {
    .my_printf = NULL,
    .tx_by_dma = NULL,
};

DECLARE_SHELL_CTX(s_usart_dbg_shell_ctx);
DECLARE_SHELL_CTX(s_usart_pl_shell_ctx);
#if (COMM_LINK_ENABLE_ISO == 1)
DECLARE_SHELL_CTX(s_usart_iso_shell_ctx);
#endif
DECLARE_COMM_CTX(s_usart_dbg_comm_ctx, AC_USART_DBG_COMM_PAYLOAD_SIZE, HOST_ADDR, USART0_LINK);
DECLARE_COMM_CTX(s_usart_pl_comm_ctx, AC_USART_PL_COMM_PAYLOAD_SIZE, HOST_ADDR, USART1_LINK);
#if (COMM_LINK_ENABLE_ISO == 1)
DECLARE_COMM_CTX(s_usart_iso_comm_ctx, AC_USART_ISO_COMM_PAYLOAD_SIZE, HOST_ADDR, USART2_LINK);
#endif
#if (COMM_LINK_ENABLE_CAN == 1)
DECLARE_COMM_CTX(s_can_dbg_comm_ctx, AC_CAN_DBG_COMM_PAYLOAD_SIZE, HOST_ADDR, CAN_DBG_LINK);
#endif

static const section_link_handler_item_t s_usart_dbg_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart_dbg_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart_dbg_comm_ctx},
};

static const section_link_handler_item_t s_usart_pl_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart_pl_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart_pl_comm_ctx},
};

#if (COMM_LINK_ENABLE_ISO == 1)
static const section_link_handler_item_t s_usart_iso_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart_iso_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart_iso_comm_ctx},
};
#endif

#if (COMM_LINK_ENABLE_CAN == 1)
static const section_link_handler_item_t s_can_dbg_handler_arr[] = {
    {.func = comm_run, .ctx = (void *)&s_can_dbg_comm_ctx},
};
#endif

REG_LINK(USART0_LINK,
         s_usart_dbg_tx_func,
         bsp_usart_dbg_rx_get_byte,
         s_usart_dbg_handler_arr,
         sizeof(s_usart_dbg_handler_arr) / sizeof(s_usart_dbg_handler_arr[0]))

#if (COMM_LINK_ENABLE_ISO == 1)
REG_LINK(USART2_LINK,
         s_usart_iso_tx_func,
         bsp_usart_iso_rx_get_byte,
         s_usart_iso_handler_arr,
         sizeof(s_usart_iso_handler_arr) / sizeof(s_usart_iso_handler_arr[0]))
#else
REG_LINK(USART2_LINK, s_usart_null_tx_func, comm_link_empty_rx_get_byte, NULL, 0u)
#endif

#if (COMM_LINK_ENABLE_CAN == 1)
REG_LINK(CAN_DBG_LINK,
         s_can_dbg_tx_func,
         bsp_can_dbg_rx_get_byte,
         s_can_dbg_handler_arr,
         sizeof(s_can_dbg_handler_arr) / sizeof(s_can_dbg_handler_arr[0]))
#else
REG_LINK(CAN_DBG_LINK, s_usart_null_tx_func, comm_link_empty_rx_get_byte, NULL, 0u)
#endif

REG_LINK(USART1_LINK,
         s_usart_pl_tx_func,
         usart_pl_rx_get_byte_cb,
         s_usart_pl_handler_arr,
         sizeof(s_usart_pl_handler_arr) / sizeof(s_usart_pl_handler_arr[0]))
REG_LINK(USART_DBG_LINK, s_usart_dbg_tx_func, comm_link_empty_rx_get_byte, NULL, 0u)
REG_LINK(USART_ISO_LINK, s_usart_iso_tx_func, comm_link_empty_rx_get_byte, NULL, 0u)
REG_LINK(UART4_LINK, s_usart_null_tx_func, comm_link_empty_rx_get_byte, NULL, 0u)
