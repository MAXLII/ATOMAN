# Zynq-7020 平台使用说明

## 1. 平台组成

`platform/zynq7020/` 是 ZYNQ MINI RevB 开发板的 Cortex-A9 + PL 平台工程，顶层只包含 `ps/` 软件工程和 `pl/` 硬件工程。目标器件为 `xc7z020clg400-2`，工具链为 Xilinx Vivado/SDK 2018.3。

平台包含：

- 可由 Tcl 完整重建的 PS7、DDR、M_AXI_GP0 和 PL 外设工程
- 运行于 PL 的 32 位定点 3P3Z IIR
- PS 可自由读写的 AXI4-Lite 32 位寄存器接口
- `code/section/baremetal/` 裸机 section 工程
- `code/section/srtos_a9/` 与 A9 SVC/IRQ 上下文切换端口
- PS UART1 的 COM6 日志、Shell 和故障诊断入口
- PL UART DMA 的 COM7 业务通信、FRAME 协议和压测入口

平台不使用第三方 RTOS。

## 2. 硬件连接

板载 CH340E 通过 PS UART1 与 Zynq 连接：

| 信号 | Zynq 引脚 | 板级链路 |
|------|-----------|----------|
| UART1 TX | MIO48 | `MIO48 -> U10(74LVC1T45) -> CH340 RXD` |
| UART1 RX | MIO49 | `CH340 TXD -> U9(74LVC1T45) -> MIO49` |

PS Bank501 为 1.8V。Vivado 工程将 `PCW_PRESET_BANK1_VOLTAGE`、MIO48 和 MIO49 设置为 LVCMOS 1.8V。该串口枚举为 COM6。

PL UART 使用 Bank 34：

| 信号 | 原理图网络 | Zynq 引脚 | 电气约束 |
|------|------------|-----------|----------|
| PL UART RX | `FPGA_GPIO_10N_34` | W15 | LVCMOS33、上拉 |
| PL UART TX | `FPGA_GPIO_11N_34` | U15 | LVCMOS33、SLEW SLOW |

PL 串口枚举为 COM7。COM6 与 COM7 均使用 921600、8 数据位、无校验、1 停止位。

## 3. PS+PL 地址空间

PS7 的 `M_AXI_GP0` 通过 AXI Interconnect 连接以下 PL 外设：

| 外设 | 基地址 | 功能 |
|------|--------|------|
| AXI PL UART DMA | `0x40600000` | PL 串口控制、DDR RX/TX ring 和错误状态 |
| AXI GPIO 输入 | `0x41200000` | PL KEY1/M19 |
| AXI GPIO 输出 | `0x41210000` | 4 个 PL LED/T12、U12、V12、W13 |
| AXI OLED DMA | `0x41220000` | DDR framebuffer DMA、SSD1306协议和串行PHY |
| AXI 3P3Z IIR | `0x43C00000` | 系数、样点、状态和历史寄存器 |

PL 时钟为 PS FCLK0 50MHz。PL UART DMA和OLED DMA控制口连接M_AXI_GP0，
两个DMA Master通过同一个AXI Interconnect连接S_AXI_HP0。UART错误IRQ连接
`IRQ_F2P[0]` / GIC SPI ID 61，OLED错误IRQ连接 `IRQ_F2P[1]` / GIC SPI ID 62。
RX ring 为 `0x1FF00000` 的64 KiB，TX ring为 `0x1FF10000` 的64 KiB，
OLED framebuffer为 `0x1FF20000` 的1024字节；
`0x1FF00000～0x1FFFFFFF` 整体保留并设置为 non-cacheable。

## 4. 3P3Z IIR

PL core 使用直接 I 型差分方程：

```text
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
       - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
```

系数为有符号 Q2.30，输入和输出为有符号 32 位整数。样点的工程量比例由 PS 决定；PL 使用 7 个并行组合乘法器和 70 位平衡加法树，计算后算术右移 30 位，并限幅到 `LIMIT_LOWER..LIMIT_UPPER`。core 从接受 `start` 到结果锁存的固定延迟为 1 个 50MHz 时钟，并支持每时钟一个样点。最终限幅值作为输出并写入反馈历史 `y[n-1]`。

32×32 位乘法器分解为 4 路 17×17 位部分积，映射到 4 个 DSP48E1。每路具有输入、乘法和输出流水寄存器。

### 4.1 AXI 寄存器

| 偏移 | 名称 | 访问 | 内容 |
|-----:|------|------|------|
| `0x00` | CONTROL | WO | bit0 启动，bit1 清状态，bit2 清完成标志 |
| `0x04` | STATUS | RO | bit0 busy，bit1 done，bit2 saturated，bit3 ready |
| `0x08` | INPUT | RW | 有符号输入样点 |
| `0x0C` | OUTPUT | RO | 有符号饱和输出样点 |
| `0x10` | B0 | RW | Q2.30 系数 b0 |
| `0x14` | B1 | RW | Q2.30 系数 b1 |
| `0x18` | B2 | RW | Q2.30 系数 b2 |
| `0x1C` | B3 | RW | Q2.30 系数 b3 |
| `0x20` | A1 | RW | Q2.30 系数 a1 |
| `0x24` | A2 | RW | Q2.30 系数 a2 |
| `0x28` | A3 | RW | Q2.30 系数 a3 |
| `0x2C` | SAMPLE_COUNT | RO | 清状态后的完成样点数 |
| `0x30` | VERSION | RO | `0x00020000` |
| `0x34` | FORMAT | RO | `0x0000201E`：32 位样点、30 位系数小数 |
| `0x38` | X1 | RO | 输入历史 x[n-1] |
| `0x3C` | X2 | RO | 输入历史 x[n-2] |
| `0x40` | X3 | RO | 输入历史 x[n-3] |
| `0x44` | Y1 | RO | 饱和输出历史 y[n-1] |
| `0x48` | Y2 | RO | 饱和输出历史 y[n-2] |
| `0x4C` | Y3 | RO | 饱和输出历史 y[n-3] |
| `0x50` | LIMIT_LOWER | RW | 有符号输出下限，复位值 `INT32_MIN` |
| `0x54` | LIMIT_UPPER | RW | 有符号输出上限，复位值 `INT32_MAX` |

AXI 写通道允许 AW 和 W 独立到达，支持 `WSTRB` 字节写。下一次 `start` 会清除旧的完成标志；软件也可显式写 CONTROL.bit2。

### 4.2 PS 调用顺序

1. 调用 `bsp_iir_configure()` 写入 7 个系数。
2. 调用 `bsp_iir_reset_state()` 清除输入、输出历史和样点计数。
3. 调用 `bsp_iir_process_sample()` 写入一个样点并等待完成。
4. 检查返回状态和饱和标志，再处理下一个样点。

`bsp_iir_self_test()` 会执行 8 点脉冲响应及正、负饱和测试。Shell 命令 `IIR_TEST` 触发相同的板上测试。

## 5. 目录职责

| 路径 | 职责 |
|------|------|
| `verilog/iir/rtl/` | 可综合 Verilog：IIR core 和 AXI4-Lite 封装 |
| `verilog/iir/sim/` | SystemVerilog testbench、仿真/综合脚本和 CSV 数值结果 |
| `verilog/iir/doc/` | PL IP 独立使用说明 |
| `verilog/uart_dma/rtl/` | 可配置 UART、同步 FIFO 和 AXI 环形 DMA RTL |
| `verilog/uart_dma/sim/` | UART DMA SystemVerilog 自检和 OOC 综合脚本 |
| `verilog/oled_dma/rtl/` | OLED 帧缓存、DMA、SSD1306 协议和串行 PHY RTL |
| `verilog/oled_dma/sim/` | OLED DMA SystemVerilog 自检和 OOC 综合脚本 |
| `platform/zynq7020/ps/` | ARM 启动、BSP、应用入口、SRTOS 端口、编译、下载和板测 |
| `platform/zynq7020/pl/` | Vivado 工程、PS7 硬件平台、自定义 IP、约束、bitstream/HDF 输出和 PL 自测 |
| `platform/zynq7020/ps/bsp/` | PS UART、PL UART DMA、共享 GIC、定时器与 IIR MMIO 平台适配 |
| `platform/zynq7020/ps/bsp/bsp_oled.c/.h` | PL OLED DMA、DDR framebuffer、显示控制和错误IRQ |
| `code/section/baremetal/section.c/.h` | 裸机 section 独立实现与统一注册接口 |
| `code/section/srtos_a9/section.c/.h` | Cortex-A9 SRTOS 独立实现与统一注册接口 |
| `platform/zynq7020/ps/srtos/a9_section_port.S` | A9 SVC/IRQ 上下文切换端口 |
| `platform/zynq7020/ps/src/main.c` | 所选 section 运行时的统一平台入口 |
| `platform/zynq7020/ps/src/platform_probe.c` | 平台自主测试与 Shell 状态命令 |
| `platform/zynq7020/ps/src/zero_player_oled.c` | 30x31 Zero Player 棋盘到 128x64 OLED 的映射 |
| `platform/zynq7020/pl/package_axi_iir_ip.tcl` | 自定义 IIR IP 与 IP-XACT 寄存器描述 |
| `platform/zynq7020/pl/package_axi_uart_dma_ip.tcl` | 自定义 PL UART DMA IP 与接口描述 |
| `platform/zynq7020/pl/package_axi_oled_dma_ip.tcl` | 自定义 PL OLED DMA IP 与寄存器描述 |
| `platform/zynq7020/pl/build_pl.tcl` | PS+PL block design、实现和输出报告 |
| `platform/zynq7020/pl/pl_selftest.tcl` | JTAG 直接访问 PL 外设的硬件测试 |

## 6. PL 独立验证

在仓库根目录执行：

```powershell
.\verilog\iir\sim\run_sim.ps1
.\verilog\iir\sim\run_synth.ps1
```

`run_sim.ps1` 先执行纯 IIR core 参考模型比对，再执行 AXI4-Lite 协议与数值测试。CSV 结果保存在 `verilog/iir/sim/`。

`run_synth.ps1` 以 50MHz 对 IIR core 做 Vivado OOC 综合，检查 DSP 映射、DRC 和时序。OOC 工程没有 PS7，因此仅豁免 Zynq 顶层规则 `ZPS7-1`；其他 DRC 均会使脚本失败。

PL UART DMA 独立验证：

```powershell
.\verilog\uart_dma\sim\run_sim.ps1
.\verilog\uart_dma\sim\run_synth.ps1
```

数字验证覆盖 UART 配置、错误、FIFO、AXI 背压、ring 回卷、错误 IRQ 和
1 MiB PRBS。

## 7. PS+PL 构建

```powershell
cd D:\OneDrive\LWX\GD32\base\platform\zynq7020
.\pl\build_pl.ps1
.\ps\compile.ps1 -Srtos 0
.\ps\compile.ps1 -Srtos 1
```

Vivado 输出位于 `pl/build/output/`：

- `zynq7020_platform.bit`
- `zynq7020_platform.hdf`
- `ps7_init.tcl`
- `drc.rpt`
- `timing_summary.rpt`
- `utilization.rpt`

无 RTOS ARM 输出位于 `ps/build/`，编译 `code/section/baremetal/section.c`；SRTOS ARM 输出位于 `ps/build_srtos/`，编译 `code/section/srtos_a9/section.c` 并链接 A9 端口。两个 Makefile 都只定义平台与工具链宏，不定义运行时选择宏。`ps/compile.ps1` 的 `-Srtos 0/1` 仅用于选择对应 Makefile 和输出目录。构建启用 `-Werror`、`-Wconversion`、`-Wsign-conversion`、`-Wshadow`、`-Wcast-align`、`-Wstrict-prototypes`、`-Wmissing-prototypes` 和 `-Wundef`。

## 8. 下载与板上测试

```powershell
# 一键完成下载、JTAG 和 COM6 检查
.\ps\board_iir_selftest.ps1

# 或分步执行
.\ps\download.ps1
.\pl\pl_selftest.ps1
```

下载脚本依次复位系统、加载 HDF、初始化 PS、配置 PL bitstream、下载所选 ELF
并启动 CPU0。`pl_selftest.ps1` 同时检查 GPIO、IIR 和 UART DMA 版本寄存器。

使用 FRAME 验证 COM6 与 COM7：

```powershell
cd D:\OneDrive\LWX\FRAME
.\frame.ps1 serial ports
.\frame.ps1 serial raw --port COM6 --baud 921600 --send-text "PL_UART_STATUS\r\n" --read-seconds 2
.\frame.ps1 serial raw --port COM7 --baud 921600 --send-hex "74 69 6D 65 0D 0A" --rx-hex --read-seconds 1

cd D:\OneDrive\LWX\GD32\base\platform\zynq7020\ps
.\board_pl_uart_dma_acceptance.ps1
```

PL UART DMA 验收脚本由两个独立进程分别独占 COM6 和 COM7，执行 11000 个
497 字节 payload 回环帧，双向总量为 11264000 字节。

Shell 命令包括 `help`、`ZYNQ_STATUS`、`IIR_TEST`、`PL_UART_STATUS`、
`PL_UART_RESET`、`PL_UART_DMA_TEST`、`PL_UART_ERROR_CLEAR` 和
`DEMO_SHELL_PING`。

## 9. section 运行模式

裸机与 A9 SRTOS 是两套独立源码，构建时通过源文件和 include path 选择。两套头文件都名为 `section.h`，注册接口保持一致。裸机模式由 SCU 私有定时器产生 10kHz 中断，在 ISR 中调用 `section_interrupt()`，主循环持续调用 `run_task()`。

SRTOS 模式使用 section 自带的任务表和调度器。Cortex-A9 端口保存通用寄存器、VFP 状态、返回 PC 与 CPSR，通过 SVC 启动/主动让出，通过 IRQ 退出路径执行时间片切换。
