#include "comm_link.h"
#include "comm_addr.h"
#include "section.h"
#include "comm.h"
#include "shell.h"
#include "bsp_usart.h"

#include <stdint.h>

#ifndef AC_USART_DBG_COMM_PAYLOAD_SIZE
#define AC_USART_DBG_COMM_PAYLOAD_SIZE 128U
#endif

static void usart_dbg_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    (void)bsp_usart_dbg_tx_dma((const uint8_t *)ptr, (uint32_t)len);
}

static section_link_tx_func_t s_usart_dbg_tx_func = {
    .my_printf = bsp_usart_dbg_printf,
    .tx_by_dma = usart_dbg_tx_by_dma_cb,
};

static shell_ctx_t s_usart_dbg_shell_ctx = {0};

DECLARE_COMM_CTX(s_usart_dbg_comm_ctx, AC_USART_DBG_COMM_PAYLOAD_SIZE, LOCAL_ADDR, USART_DBG_LINK)

static const section_link_handler_item_t s_usart_dbg_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart_dbg_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart_dbg_comm_ctx},
};

REG_LINK(USART_DBG_LINK,
         s_usart_dbg_tx_func,
         bsp_usart_dbg_rx_get_byte,
         s_usart_dbg_handler_arr,
         sizeof(s_usart_dbg_handler_arr) / sizeof(s_usart_dbg_handler_arr[0]))
