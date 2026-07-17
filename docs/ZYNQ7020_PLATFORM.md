# Zynq-7020 平台使用说明

## 1. 平台组成

`platform/zynq7020/` 是 ZYNQ MINI RevB 开发板的 Cortex-A9 + PL 平台工程。目标器件为 `xc7z020clg400-2`，工具链为 Xilinx Vivado/SDK 2018.3。

平台包含：

- 可由 Tcl 完整重建的 PS7、DDR、M_AXI_GP0 和 PL 外设工程
- 运行于 PL 的 32 位定点 3P3Z IIR
- PS 可自由读写的 AXI4-Lite 32 位寄存器接口
- Cortex-A9 无 RTOS section 工程
- section 自带 SRTOS 的 Cortex-A9 上下文切换端口
- PS UART1 的 COM5 通信、FRAME 二进制协议和 Shell 调试入口

平台不使用第三方 RTOS。

## 2. 硬件连接

板载 CH340E 通过 PS UART1 与 Zynq 连接：

| 信号 | Zynq 引脚 | 板级链路 |
|------|-----------|----------|
| UART1 TX | MIO48 | `MIO48 -> U10(74LVC1T45) -> CH340 RXD` |
| UART1 RX | MIO49 | `CH340 TXD -> U9(74LVC1T45) -> MIO49` |

PS Bank501 为 1.8V。Vivado 工程将 `PCW_PRESET_BANK1_VOLTAGE`、MIO48 和 MIO49 设置为 LVCMOS 1.8V。COM5 参数为 115200、8 数据位、无校验、1 停止位。

## 3. PS+PL 地址空间

PS7 的 `M_AXI_GP0` 通过 AXI Interconnect 连接以下 PL 外设：

| 外设 | 基地址 | 功能 |
|------|--------|------|
| AXI UART Lite | `0x40600000` | 扩展 PL 串口，115200 8N1 |
| AXI GPIO 输入 | `0x41200000` | PL KEY1/M19 |
| AXI GPIO 输出 | `0x41210000` | 4 个 PL LED/T12、U12、V12、W13 |
| AXI 3P3Z IIR | `0x43C00000` | 系数、样点、状态和历史寄存器 |

PL 时钟为 PS FCLK0 50MHz。板载 COM5 使用 PS UART1，不经过 AXI UART Lite。

## 4. 3P3Z IIR

PL core 使用直接 I 型差分方程：

```text
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
       - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
```

系数为有符号 Q2.30，输入和输出为有符号 32 位整数。样点的工程量比例由 PS 决定；PL 使用 70 位累加器，计算后算术右移 30 位，并饱和到 `int32_t`。一个 core 样点从接受 `start` 到 `done` 的固定延迟为 13 个 50MHz 时钟。

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
| `0x30` | VERSION | RO | `0x00010000` |
| `0x34` | FORMAT | RO | `0x0000201E`：32 位样点、30 位系数小数 |
| `0x38` | X1 | RO | 输入历史 x[n-1] |
| `0x3C` | X2 | RO | 输入历史 x[n-2] |
| `0x40` | X3 | RO | 输入历史 x[n-3] |
| `0x44` | Y1 | RO | 饱和输出历史 y[n-1] |
| `0x48` | Y2 | RO | 饱和输出历史 y[n-2] |
| `0x4C` | Y3 | RO | 饱和输出历史 y[n-3] |

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
| `verilog/iir/src/` | 可综合 Verilog：IIR core 和 AXI4-Lite 封装 |
| `verilog/iir/sim/` | SystemVerilog testbench、仿真/综合脚本和 CSV 数值结果 |
| `verilog/iir/doc/` | PL IP 独立使用说明 |
| `platform/zynq7020/bsp/` | UART、定时器、GIC 与 IIR MMIO 平台适配 |
| `platform/zynq7020/srtos/section_a9.c` | A9 无 RTOS 与 SRTOS 的 section 双模式实现 |
| `platform/zynq7020/srtos/a9_section_port.S` | A9 SVC/IRQ 上下文切换端口 |
| `platform/zynq7020/src/main.c` | 无 RTOS/SRTOS 条件入口 |
| `platform/zynq7020/src/platform_probe.c` | 平台自主测试与 Shell 状态命令 |
| `platform/zynq7020/pl/package_axi_iir_ip.tcl` | 自定义 IIR IP 与 IP-XACT 寄存器描述 |
| `platform/zynq7020/pl/build_pl.tcl` | PS+PL block design、实现和输出报告 |
| `platform/zynq7020/pl_selftest.tcl` | JTAG 直接访问 PL 外设的硬件测试 |

## 6. PL 独立验证

在仓库根目录执行：

```powershell
.\verilog\iir\sim\run_sim.ps1
.\verilog\iir\sim\run_synth.ps1
```

`run_sim.ps1` 先执行纯 IIR core 参考模型比对，再执行 AXI4-Lite 协议与数值测试。CSV 结果保存在 `verilog/iir/sim/`。

`run_synth.ps1` 以 50MHz 对 IIR core 做 Vivado OOC 综合，检查 DSP 映射、DRC 和时序。OOC 工程没有 PS7，因此仅豁免 Zynq 顶层规则 `ZPS7-1`；其他 DRC 均会使脚本失败。

## 7. PS+PL 构建

```powershell
cd D:\OneDrive\LWX\GD32\base\platform\zynq7020
.\pl\build_pl.ps1
.\compile.ps1 -Srtos 0
.\compile.ps1 -Srtos 1
```

Vivado 输出位于 `pl/build/output/`：

- `zynq7020_platform.bit`
- `zynq7020_platform.hdf`
- `ps7_init.tcl`
- `drc.rpt`
- `timing_summary.rpt`
- `utilization.rpt`

无 RTOS ARM 输出位于 `build/`，SRTOS ARM 输出位于 `build_srtos/`。两个工程均编译 `section_a9.c`，平台配置头分别定义 `SRTOS` 为 0 和 1；Makefile 只定义平台与工具链宏。构建启用 `-Werror`、`-Wconversion`、`-Wsign-conversion`、`-Wshadow`、`-Wcast-align`、`-Wstrict-prototypes`、`-Wmissing-prototypes` 和 `-Wundef`。

## 8. 下载与板上测试

```powershell
# 一键完成下载、JTAG 和 COM5 检查
.\board_iir_selftest.ps1

# 或分步执行
.\download.ps1
.\pl_selftest.ps1
```

下载脚本依次复位系统、加载 HDF、初始化 PS、配置 PL bitstream、下载无 RTOS ELF 并启动 CPU0。`board_iir_selftest.ps1` 串联下载、JTAG PL 自测和 COM5 的 `IIR_TEST`/`ZYNQ_STATUS` 检查，最终通过标志为 `BOARD_IIR_SELFTEST result=PASS`。

使用 FRAME 验证 COM5：

```powershell
cd D:\OneDrive\LWX\FRAME
.\frame.ps1 serial ports
.\frame.ps1 serial raw --port COM5 --baud 115200 --send-text "IIR_TEST\r\n" --read-seconds 2
.\frame.ps1 serial raw --port COM5 --baud 115200 --send-text "ZYNQ_STATUS\r\n" --read-seconds 2
```

Shell 命令包括 `help`、`ZYNQ_STATUS`、`IIR_TEST` 和 `DEMO_SHELL_PING`。

## 9. section 运行模式

`section_a9.c` 通过平台配置头中的 `SRTOS` 宏选择运行模式。无 RTOS 模式由 SCU 私有定时器产生 10kHz 中断，在 ISR 中调用 `section_interrupt()`，主循环持续调用 `run_task()`。

SRTOS 模式使用 section 自带的任务表和调度器。Cortex-A9 端口保存通用寄存器、VFP 状态、返回 PC 与 CPSR，通过 SVC 启动/主动让出，通过 IRQ 退出路径执行时间片切换。
