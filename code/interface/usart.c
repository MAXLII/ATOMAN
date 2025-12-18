#include "usart.h"
#include "bsp_usart.h"
#include "section.h"
#include <stdarg.h>
#include "gd32g5x3.h"

USART_LINK_E usart_link = USART0_LINK;

void usart0_printf(const char *__format, ...)
{
    usart_link = USART0_LINK; // 设置当前使用的 USART 链接
    va_list args;
    va_start(args, __format);
    vprintf(__format, args);
    va_end(args);
}

void (*my_func_arr[])(uint8_t, DEC_MY_PRINTF, void *) = {
    shell_run,
    comm_run,
};

section_link_tx_func_t ut1_printf = {
    .my_printf = usart0_printf,
    .tx_by_dma = NULL,
};

REG_LINK(USART0_LINK,
         128,
         ut1_printf,
         (uint32_t *)&DMA_CH0CNT(DMA0),
         my_func_arr,
         sizeof(my_func_arr) / sizeof(my_func_arr[0]),
         0x02);
