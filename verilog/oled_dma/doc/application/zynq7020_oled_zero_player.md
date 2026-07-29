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

## 3. BSP 使用

`bsp_oled_init()`完成版本和画面尺寸核对、non-cacheable内存属性设置、默认
寄存器配置、错误IRQ连接及SSD1306初始化。初始化成功后可以直接修改
framebuffer：

```c
if (bsp_oled_init() == XST_SUCCESS)
{
    bsp_oled_frame_clear();
    bsp_oled_pixel_set(0U, 0U, 1U);
    bsp_oled_pixel_set(127U, 63U, 1U);
    bsp_oled_present();
}
```

| 接口 | 行为 |
|---|---|
| `bsp_oled_frame_clear()` | 清空DDR framebuffer，不立即刷新屏幕 |
| `bsp_oled_pixel_set()` | 修改一个像素，越界坐标被忽略 |
| `bsp_oled_present()` | 等待外设空闲，提交一帧并等待`FRAME_COUNT`变化 |
| `bsp_oled_display_clear()` | 让PL直接发送全零画面，不修改DDR |
| `bsp_oled_display_enable()` | 打开或关闭显示 |
| `bsp_oled_invert_set()` | 设置正常或反显 |
| `bsp_oled_contrast_set()` | 设置8位对比度 |
| `bsp_oled_auto_refresh_set()` | 配置自动刷新周期；启用后立即返回 |
| `bsp_oled_status_get()` | 读取硬件状态、计数器和IRQ诊断信息 |
| `bsp_oled_reset()` | 软复位PL外设并重新初始化和刷新 |

framebuffer位于共享的1MiB non-cacheable窗口。`bsp_oled_present()`在提交
PRESENT前执行DMB，确保之前的像素写入先于DMA读取。该接口返回`void`，不能
向调用方直接返回轮询超时；硬件报告的DMA停止和错误计数可通过
`bsp_oled_status_get()`读取。

## 4. Zero Player映射

30行、31列棋盘通过整数边界缩放覆盖完整128x64区域：

```text
x_begin = column * 128 / 31
x_end   = (column + 1) * 128 / 31
y_begin = row * 64 / 30
y_end   = (row + 1) * 64 / 30
```

应用只修改DDR framebuffer并调用 `bsp_oled_present()`。PL自主读取完整帧快照，
因此DMA完成后应用可以继续修改DDR，不影响正在串行发送的画面。

## 5. 错误恢复

framebuffer地址不满足1024字节对齐、SPI分频或自动刷新周期非法时，PL置位
配置错误。AXI读取失败会停止后续DMA刷新并保存`DMA_STOP_REASON`。错误IRQ
只负责锁存和清除错误位，不在中断中等待显示操作。

恢复时先调用`bsp_oled_status_get()`保存诊断信息，再调用`bsp_oled_reset()`。
软复位不会清除硬件累计计数器，因此可以比较复位前后的计数定位重复故障。

## 6. 验证

- baremetal和A9 SRTOS严格构建通过。
- Zero Player每1000 ms生成并刷新一帧。
- baremetal与SRTOS均完成1000次手动刷新。
- 100 ms与1000 ms自动刷新通过。
- 正常路径IRQ、AXI错误、命令错误和DMA停止原因均为0。
- 实体OLED的最终光学显示由开发板屏幕直接观察确认。

RTL验证入口为`verilog/oled_dma/sim/run_sim.ps1`。当前自检覆盖1024帧协议
连续发送、AXI4-Lite访问、16次burst快照、自动刷新、清屏、地址未对齐和
AXI错误注入。`run_synth.ps1`执行50MHz OOC综合；它验证模块自身的DRC和
时序，不代表完整平台布局布线结果。

## 7. 关联导航

- 源码：[OLED DMA顶层](../../rtl/axi_oled_dma.v) · [Zynq OLED BSP](../../../../platform/zynq7020/ps/bsp/bsp_oled.c) · [Zero Player应用](../../../../platform/zynq7020/ps/src/zero_player_oled.c)
- 设计：[PL OLED DMA设计](../design/zynq7020_pl_oled_dma_design.md)
