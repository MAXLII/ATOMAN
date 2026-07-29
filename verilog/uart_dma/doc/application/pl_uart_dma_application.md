# PL UART DMA 应用说明

## 平台连接

PL UART DMA 在 Zynq-7020 平台使用 `0x40600000` 控制寄存器窗口，通过 `M_AXI_GP0` 配置，通过 `S_AXI_HP0` 访问 DDR。COM7 对应 PL UART，配置为 921600 8N1；COM6 保留为 PS 调试串口。

默认 DDR 区域：

- RX ring：`0x1FF00000`，64 KiB。
- TX ring：`0x1FF10000`，64 KiB。

## BSP 接口

应用层通过 `platform/zynq7020/ps/bsp/bsp_usart.h` 中的 PL UART 接口使用环形 DMA：

- `bsp_usart_pl_init()`
- `bsp_usart_pl_configure()`
- `bsp_usart_pl_tx_dma()`
- `bsp_usart_pl_printf()`
- `bsp_usart_pl_rx_get_byte()`
- `bsp_usart_pl_status_get()`
- `bsp_usart_pl_reset()`

BSP 负责 DDR 地址、环形回卷、生产者/消费者计数同步、内存屏障和错误恢复。应用层不直接维护 DMA 指针。

## 运行规则

正常 RX、TX、DMA 完成、RX 水位和 TX 空不启用中断。错误中断只用于 parity、frame、overflow、非法配置和 AXI 错误，ISR 只记录并清除状态。

`USART1_LINK` 使用 COM7，`USART0_LINK` 和 `USART_DBG_LINK` 使用 COM6。两个串口具有独立的 Shell 与通信上下文。

## 关联导航

- 源码：[UART DMA顶层](../../rtl/axi_uart_dma.v) · [Zynq USART BSP](../../../../platform/zynq7020/ps/bsp/bsp_usart.c)
- 设计：[PL UART DMA设计](../design/zynq7020_pl_uart_dma_design.md)
