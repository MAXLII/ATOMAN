#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include "hc32_ll.h"
#include "hc32_ll_mcan.h"

#ifndef BSP_CAN_AUTO_INIT
#define BSP_CAN_AUTO_INIT               (1U)
#endif

#ifndef BSP_CAN_RX_BUF_SIZE
#define BSP_CAN_RX_BUF_SIZE             (256U)
#endif

#ifndef BSP_CAN_DBG_PRINTF_BUF_SIZE
#define BSP_CAN_DBG_PRINTF_BUF_SIZE     (256U)
#endif

#ifndef BSP_CAN_TX_DMA_RING_SIZE
#define BSP_CAN_TX_DMA_RING_SIZE        (512U)
#endif

#ifndef BSP_CAN_TX_WAIT_TIMEOUT
#define BSP_CAN_TX_WAIT_TIMEOUT         (0x00FFFFFFUL)
#endif

#ifndef BSP_CAN_TEST_ENABLE
#define BSP_CAN_TEST_ENABLE             (0U)
#endif

#ifndef BSP_CAN_TX_PORT
#define BSP_CAN_TX_PORT                 (GPIO_PORT_B)
#endif

#ifndef BSP_CAN_TX_PIN
#define BSP_CAN_TX_PIN                  (GPIO_PIN_11)
#endif

#ifndef BSP_CAN_TX_PIN_FUNC
#define BSP_CAN_TX_PIN_FUNC             (GPIO_FUNC_56)
#endif

#ifndef BSP_CAN_RX_PORT
#define BSP_CAN_RX_PORT                 (GPIO_PORT_B)
#endif

#ifndef BSP_CAN_RX_PIN
#define BSP_CAN_RX_PIN                  (GPIO_PIN_10)
#endif

#ifndef BSP_CAN_RX_PIN_FUNC
#define BSP_CAN_RX_PIN_FUNC             (GPIO_FUNC_57)
#endif

#ifndef BSP_CAN_PHY_STBY_PORT
#define BSP_CAN_PHY_STBY_PORT           (GPIO_PORT_B)
#endif

#ifndef BSP_CAN_PHY_STBY_PIN
#define BSP_CAN_PHY_STBY_PIN            (GPIO_PIN_02)
#endif

void bsp_can_dbg_printf(const char *__format, ...);
void bsp_can_dbg_tx(char *ptr, int len);
uint8_t bsp_can_dbg_rx_get_byte(uint8_t *p_data);

#endif
