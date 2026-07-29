# Zynq-7020 PL UART 环形 DMA 设计

## 1. 端口与用途

ZYNQ MINI RevB 平台保留两个相互独立的串口：

| 端口 | 实现 | 参数 | 用途 |
|------|------|------|------|
| COM6 | PS UART1，MIO48/MIO49 | 921600 8N1 | 日志、Shell、状态和故障诊断 |
| COM7 | PL UART DMA | 921600 8N1 | 业务通信、`USART1_LINK`、FRAME 压测 |

PL 端 Bank 34 使用 LVCMOS33。原理图网络与器件引脚的实际对应关系为：

| PL 信号 | 原理图网络 | XC7Z020 引脚 | 约束 |
|---------|------------|--------------|------|
| RX | `FPGA_GPIO_10N_34` | W15 / IO_L10N_34 | 上拉 |
| TX | `FPGA_GPIO_11N_34` | U15 / IO_L11N_34 | SLEW SLOW |

## 2. RTL 组成

RTL 位于 `verilog/uart_dma/`：

| 文件 | 职责 |
|------|------|
| `rtl/uart_sync_fifo.v` | 16 字节同步 FIFO |
| `rtl/uart_serial_core.v` | 16 倍采样 UART TX/RX、配置、错误检测和内部回环 |
| `rtl/axi_uart_dma.v` | AXI4-Lite 控制、AXI Master DMA、DDR 环形缓冲和错误 IRQ |
| `sim/tb_uart_serial_core.sv` | UART 参数组合、采样、错误和连续帧自检 |
| `sim/tb_axi_uart_dma.sv` | AXI、ring、错误、中断和 1 MiB PRBS 自检 |

UART 支持 5～8 数据位、无/奇/偶校验、1/2 停止位和
1200～1000000 baud。50 MHz 时钟下由 32 位相位累加器生成 16 倍采样使能：

```text
baud_inc = round(baud × 16 × 2^32 / 50000000)
```

921600 baud 的默认相位增量为 `1266637395`。RX 输入先经过双触发器同步，
再进行起始位确认和中心采样。UART 错误包括 parity、frame 和 break。

## 3. 环形 DMA

RX 与 TX 分别使用独立的 DDR 环形缓冲。缓冲大小为 2 的幂，合法范围为
256～65536 字节。

| 通道 | 软件维护 | 硬件维护 | 数据方向 |
|------|----------|----------|----------|
| RX | `rx_consumed` | `rx_produced` | UART RX FIFO → DDR |
| TX | `tx_produced` | `tx_consumed` | DDR → UART TX FIFO |

地址偏移由 `count & (size - 1)` 产生，32 位计数器自然回卷。RX ring 满时保留
旧数据、丢弃新数据并增加 overflow 计数；TX ring 空是正常空闲状态。

AXI Master 地址和数据均为 32 位，单 outstanding，AW/W 可独立握手，写通道
使用 `WSTRB`。读写均支持背压。收到 SLVERR 或 DECERR 后，对应 DMA 通道停止，
错误和停止原因保持到软件清除或复位。

PS 使用以下 non-cacheable DDR 区域：

| 区域 | 地址 | 大小 |
|------|------|-----:|
| RX ring | `0x1FF00000` | 64 KiB |
| TX ring | `0x1FF10000` | 64 KiB |
| 保留区 | `0x1FF00000～0x1FFFFFFF` | 1 MiB |

链接脚本断言固件末地址不超过 `0x1FF00000`。BSP 内部完成 MMU 属性设置、内存
屏障、空间等待、计数器发布、回卷和错误恢复，应用层不直接处理 DDR 地址或
DMA 指针。

## 4. AXI4-Lite 寄存器

控制基地址为 `0x40600000`。

| 偏移 | 名称 | 功能 |
|-----:|------|------|
| `0x00` | CONTROL | enable、soft reset、internal loopback |
| `0x04` | STATUS | UART、FIFO、ring 和 DMA 状态 |
| `0x08` | UART_CONFIG | 数据位、校验、停止位 |
| `0x0C` | BAUD_INCREMENT | 波特率相位增量 |
| `0x10` | RX_BASE | RX ring DDR 基址 |
| `0x14` | RX_SIZE | RX ring 大小 |
| `0x18` | RX_PRODUCED | RX 硬件生产计数 |
| `0x1C` | RX_CONSUMED | RX 软件消费计数 |
| `0x20` | TX_BASE | TX ring DDR 基址 |
| `0x24` | TX_SIZE | TX ring 大小 |
| `0x28` | TX_PRODUCED | TX 软件生产计数 |
| `0x2C` | TX_CONSUMED | TX 硬件消费计数 |
| `0x30` | IRQ_STATUS | W1C 错误中断状态 |
| `0x34` | IRQ_ENABLE | 错误中断使能 |
| `0x38` | UART_ERROR_COUNT | UART 错误计数 |
| `0x3C` | DMA_ERROR_COUNT | DMA 错误计数 |
| `0x40` | DMA_STOP_REASON | DMA 停止原因 |
| `0x44` | VERSION | `0x00010000` |

正常 RX、TX、DMA 完成、RX 水位和 TX 空均不产生中断。IRQ 只覆盖 parity、
frame、overflow、非法 ring 配置和 AXI 错误，通过 `IRQ_F2P[0]` 接入 GIC
SPI ID 61。ISR 只记录并 W1C 清除错误，不打印、不收发、不解析协议。

## 5. PS 接口与链路

`bsp_usart.c/.h` 提供：

- `bsp_usart_pl_init()`
- `bsp_usart_pl_configure()`
- `bsp_usart_pl_tx_dma()`
- `bsp_usart_pl_printf()`
- `bsp_usart_pl_rx_get_byte()`
- `bsp_usart_pl_status_get()`
- `bsp_usart_pl_reset()`
- `bsp_usart_pl_error_clear()`

`USART0_LINK` 和 `USART_DBG_LINK` 使用 COM6；`USART1_LINK` 使用 COM7。COM6 与
COM7 分别具有独立的 Shell 和 comm context。板级命令包括
`PL_UART_STATUS`、`PL_UART_RESET`、`PL_UART_DMA_TEST` 和
`PL_UART_ERROR_CLEAR`。

FRAME 兼容的最大 payload 回环命令为 `cmd_set=0x01`、
`cmd_word=0x17`。命令直接 ACK 同一命令号并原样返回 payload；目标最大有效
payload 为 497 字节，对应 512 字节完整协议帧。原有 `0x30/0x01` demo 回环
命令保持不变。
