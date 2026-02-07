#include "usart.h"
#include "bsp_usart.h"
#include "section.h"
#include <stdarg.h>
#include "gd32g5x3.h"
#include "comm.h"
#include "shell.h"
#include <stdio.h>

USART_LINK_E usart_link = USART0_LINK;

void usart0_printf(const char *__format, ...)
{
    usart_link = USART0_LINK; // 设置当前使用的 USART 链接
    va_list args;
    va_start(args, __format);
    vprintf(__format, args);
    va_end(args);
}

section_link_tx_func_t ut1_printf = {
    .my_printf = usart0_printf,
    .tx_by_dma = NULL,
};

/* ===================== ctx（由本文件显式持有） ===================== */
static shell_ctx_t ut0_shell_ctx = {0};

/* COMM ctx：配套 payload buffer（长度可按需调整） */
DECLARE_COMM_CTX(ut0_comm_ctx, 128, 0x02, USART0_LINK);

/* ===================== handler_arr：函数 + ctx ===================== */
static const section_link_handler_item_t ut0_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&ut0_shell_ctx},
    {.func = comm_run, .ctx = (void *)&ut0_comm_ctx},
};

/* ===================== link 注册 ===================== */
REG_LINK(USART0_LINK,
         ut1_printf,
         (uint32_t *)&DMA_CH0CNT(DMA0),
         ut0_handler_arr,
         sizeof(ut0_handler_arr) / sizeof(ut0_handler_arr[0]));
