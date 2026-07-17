# Zynq-7020 平台自测报告

## 1. 测试信息

| 项目 | 内容 |
|------|------|
| 测试日期 | 2026-07-17 |
| 开发板 | ZYNQ MINI RevB，XC7Z020-CLG400-2 |
| 串口 | COM5，CH340，115200 8N1 |
| FPGA 工具 | Vivado 2018.3 |
| ARM 工具 | Xilinx SDK 2018.3 GNU Arm |
| RTL 仿真 | Icarus Verilog，SystemVerilog 2012 |
| 上位机 | FRAME CLI |

本报告中的板上串口原始输出采集于 section 目录拆分前，因此保留当时的 `no-RTOS` 和 `section-SRTOS` 模式文本。当前源码已改为独立的 `baremetal` 与 `srtos_a9` 实现，板测脚本对应匹配 `baremetal` 和 `srtos-a9`。

## 2. 当前结论

3P3Z IIR 的纯 PL 数字验证、AXI4-Lite 仿真、OOC 综合、完整 PS+PL 实现和板上 AXI 访问均已通过。A9 无 RTOS 与 section SRTOS 固件均通过严格构建、JTAG 下载和 COM5 双向通信验证。SRTOS 公共栈任务切换期间无调度故障、现场保存失败或现场释放失败。

| 测试项 | 告警 | 错误 | 状态 |
|--------|-----:|-----:|------|
| IIR core 参考模型，322 个样点 | 0 | 0 | PASS |
| AXI RTL，10 个样点 | 0 | 0 | PASS |
| IIR core OOC 综合 | 0 | 0 | PASS |
| 完整 PS+PL synth/impl | 0 | 0 | PASS |
| ARM 无 RTOS 严格构建 | 0 | 0 | PASS |
| ARM section SRTOS 严格构建 | 0 | 0 | PASS |
| 板上 JTAG IIR 自测 | 0 | 0 | PASS |
| 当前平台 COM5 RX/TX | 0 | 0 | PASS |
| section SRTOS 公共栈调度 | 0 | 0 | PASS |

## 3. PL 数字验证

独立 core testbench 对每个 RTL 输出执行参考模型比对：

```text
CORE_SIM_RESULT PASS vectors=322 latency=13 failures=0
```

覆盖 64 个乘法边界组合、256 个连续 PRBS 状态反馈样点和正/负饱和。全部样点的输出、饱和标志、X/Y 历史和完成延迟一致。

AXI testbench 结果：

```text
SIM_RESULT PASS vectors=10 failures=0
```

覆盖 AW 先到、W 先到、AW/W 同时到达、`WSTRB`、寄存器读回、8 点脉冲响应和正/负饱和。CSV 中 332 个总样点均为 `expected == actual`。

详细数据见 `docs/ZYNQ7020_IIR_PL_VERIFICATION.md` 和 `verilog/iir/sim/*.csv`。

## 4. Vivado OOC 综合

目标为 `xc7z020clg400-2`，PL 时钟约束为 50MHz：

| 项目 | 结果 |
|------|------|
| WNS | +15.599ns |
| TNS | 0ns |
| LUT | 373 |
| 寄存器 | 634 |
| DSP48E1 | 4 |
| DSP 输入/输出流水线 DRC | 0 |

OOC 报告包含一条预期的 `ZPS7-1`，表示独立 PL core 顶层没有 PS7。脚本只豁免该 Zynq 顶层规则，任何其他 DRC 都会失败。

## 5. 完整 PS+PL 实现

完整 block design 包含 PS7、DDR、AXI Interconnect、AXI UART Lite、两个 AXI GPIO 和自定义 AXI IIR IP。

| 项目 | 结果 |
|------|------|
| DRC violations | 0 |
| WNS | +13.019ns |
| TNS | 0ns |
| Slice LUT | 1491 / 53200，2.80% |
| Slice Register | 2028 / 106400，1.91% |
| DSP48E1 | 4 / 220，1.82% |
| 实现日志 | 0 warning，0 critical warning，0 error |
| bitstream | 已生成 |
| HDF | 已生成 |

## 6. ARM 无 RTOS 构建

当前无 RTOS Makefile 只定义 `IS_ZYNQ7020` 与 `TOOLCHAIN_GCC`，编译 `code/section/baremetal/section.c`，并优先包含同目录的 `section.h`。该工程不链接 SVC/IRQ 上下文切换端口；ELF 使用 Xilinx standalone 向量表 `_vector_table`。

板测版本记录的构建尺寸：

| text | data | bss | total |
|-----:|-----:|----:|------:|
| 132100 | 16812 | 28672 | 177584 |

链接段：

```text
.section_registry 0x0012F010 size 0x3C0
.func_ram         0x0012F3D0 size 0xF4
```

编译启用 `-Werror`、`-Wpedantic`、`-Wconversion`、`-Wsign-conversion`、`-Wshadow`、`-Wcast-align`、`-Wstrict-prototypes`、`-Wmissing-prototypes` 和 `-Wundef`，结果为 0 编译告警、0 错误。

## 7. ARM section SRTOS 构建

当前 A9 SRTOS Makefile 仍只定义 `IS_ZYNQ7020` 与 `TOOLCHAIN_GCC`，编译独立的 `code/section/srtos_a9/section.c`，并链接 `a9_section_control.c` 与 `a9_section_port.S`。A9 端口保存通用寄存器、VFP D0-D31、FPEXC、FPSCR、返回 PC 与 CPSR。

板测版本记录的构建尺寸：

| text | data | bss | total |
|-----:|-----:|----:|------:|
| 135888 | 17532 | 35024 | 188444 |

链接段：

```text
.section_registry 0x001308E0 size 0x3E0
.func_ram         0x00130CC0 size 0xF4
```

严格编译结果为 0 编译告警、0 错误。A9 SRTOS ELF 包含 `a9_section_irq_handler` 与 `a9_section_port_yield`；裸机 ELF 不链接这两个 A9 端口符号。三套 section 头文件都保留统一的调度兼容函数声明，裸机实现对应空操作。

## 8. 板上联调结果

板载 PS UART1 使用 MIO48/MIO49，通过 CH340 映射为 COM5。无 RTOS 镜像下载后，PL AXI 自测结果为：

```text
DOWNLOAD_RESULT status=PASS srtos=0
PL_SELFTEST result=PASS gpio_input=0x00000001 gpio_output=0x00000005
iir_impulse=524288 131072 32768 8192 4294952960 512 1152 544
iir_saturation=0x7FFFFFFF/0x80000000 iir_version=0x00010000
```

FRAME 通过 COM5 发送 `IIR_TEST` 与 `ZYNQ_STATUS`，目标端正确接收命令并返回：

```text
iir result=PASS impulse=524288,131072,32768,8192,-14336,512,1152,544
zynq mode=no-RTOS tick_100us=77831 task_count=77 section=0012F010-0012F3D0
BOARD_IIR_SELFTEST result=PASS port=COM5 baud=115200 mode=no-RTOS
```

随后下载 SRTOS 镜像。两次状态采样之间平台任务计数从 26 增长到 83，100ms 与 123ms 长任务均持续输出；公共现场池和公共运行栈状态正常：

```text
zynq mode=section-SRTOS tick_100us=26563 task_count=26 srtos=1
fault=0 save_fail=0 release_fail=0 pool=198/1024 stack_free=430

zynq mode=section-SRTOS tick_100us=83775 task_count=83 srtos=1
fault=0 save_fail=0 release_fail=0 pool=202/1024 stack_free=430

BOARD_SRTOS_SELFTEST result=PASS port=COM5 baud=115200
task_count=26->83 fault=0 save_fail=0 release_fail=0 iir=PASS
```

复测命令：

```powershell
cd D:\OneDrive\LWX\GD32\base\platform\zynq7020
.\ps\board_iir_selftest.ps1
.\ps\board_srtos_selftest.ps1
```
