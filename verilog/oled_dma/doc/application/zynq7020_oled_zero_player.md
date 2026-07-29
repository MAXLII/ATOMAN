# Zynq-7020 OLED Zero Player 应用与验证

## 1. 硬件连接

ZYNQ MINI RevB 的 J4 为128x64单色OLED，PL驱动4线串行接口：

| 信号 | 功能 | XC7Z020引脚 |
|------|------|-------------|
| D0 | 串行时钟 | E18 |
| D1 | 串行数据 | E19 |
| DC | 数据/命令选择 | F16 |
| RST | 低有效复位 | F17 |

Bank 35使用LVCMOS33和SLEW SLOW。

## 2. PL OLED DMA

OLED由基地址 `0x41220000` 的自定义外设驱动。PS framebuffer位于
`0x1FF20000`，大小1024字节，位于non-cacheable DDR保留区。

PL外设将数据控制、SSD1306协议和串行PHY分开：

- 数据控制层完成寄存器、DDR DMA、帧快照和错误处理。
- 协议层完成初始化、寻址、清屏和显示命令。
- PHY层只完成命令/数据字节的4线串行发送。

## 3. Zero Player映射

30行、31列棋盘通过整数边界缩放覆盖完整128x64区域：

```text
x_begin = column * 128 / 31
x_end   = (column + 1) * 128 / 31
y_begin = row * 64 / 30
y_end   = (row + 1) * 64 / 30
```

应用只修改DDR framebuffer并调用 `bsp_oled_present()`。PL自主读取完整帧快照，
因此DMA完成后应用可以继续修改DDR，不影响正在串行发送的画面。

## 4. 验证

- baremetal和A9 SRTOS严格构建通过。
- Zero Player每1000 ms生成并刷新一帧。
- baremetal与SRTOS均完成1000次手动刷新。
- 100 ms与1000 ms自动刷新通过。
- 正常路径IRQ、AXI错误、命令错误和DMA停止原因均为0。
- 实体OLED的最终光学显示由开发板屏幕直接观察确认。

## 5. 关联导航

- 源码：[OLED DMA顶层](../../rtl/axi_oled_dma.v) · [Zynq OLED BSP](../../../../platform/zynq7020/ps/bsp/bsp_oled.c) · [Zero Player应用](../../../../platform/zynq7020/ps/src/zero_player_oled.c)
- 设计：[PL OLED DMA设计](../design/zynq7020_pl_oled_dma_design.md)
