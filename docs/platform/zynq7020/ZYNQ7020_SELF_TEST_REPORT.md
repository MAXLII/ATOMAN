# Zynq-7020 平台自测报告

## 1. 测试信息

| 项目 | 内容 |
|------|------|
| 测试日期 | 2026-07-25 |
| 开发板 | ZYNQ MINI RevB，XC7Z020-CLG400-2 |
| 串口 | COM6（PS UART1），921600 8N1 |
| FPGA 工具 | Vivado 2018.3 |
| ARM 工具 | Xilinx SDK 2018.3 GNU Arm |
| RTL 仿真 | Icarus Verilog，SystemVerilog 2012 |
| 上位机 | FRAME CLI |

## 2. 当前结论

单周期 3P3Z IIR 的纯 PL 数字验证、AXI4-Lite 仿真、OOC 综合、完整 PS+PL 实现和板上 AXI 访问均已通过。baremetal 与 A9 SRTOS 固件均通过严格构建、JTAG 下载和 COM6 双向通信验证。SRTOS 公共栈任务切换期间无调度故障、现场保存失败或现场释放失败。

| 测试项 | 告警 | 错误 | 状态 |
|--------|-----:|-----:|------|
| IIR core 参考模型，325 个样点 | 0 | 0 | PASS |
| AXI RTL，13 个样点 | 0 | 0 | PASS |
| IIR core OOC 综合 | 0 | 0 | PASS |
| 完整 PS+PL synth/impl | 0 | 0 | PASS |
| ARM 无 RTOS 严格构建 | 0 | 0 | PASS |
| ARM section SRTOS 严格构建 | 0 | 0 | PASS |
| 板上 JTAG IIR 自测 | 0 | 0 | PASS |
| 当前平台 COM6 RX/TX | 0 | 0 | PASS |
| section SRTOS 公共栈调度 | 0 | 0 | PASS |

## 3. PL 数字验证

独立 core testbench 对每个 RTL 输出执行参考模型比对：

```text
CORE_SIM_RESULT PASS vectors=325 latency=1 failures=0
```

覆盖 64 个乘法边界组合、256 个连续 PRBS 状态反馈样点、正/负饱和以及配置上下限和限幅历史反馈。全部样点的输出、限幅标志、X/Y 历史和单周期完成延迟一致，并验证连续每时钟一个样点。

AXI testbench 结果：

```text
SIM_RESULT PASS vectors=13 failures=0
```

覆盖 AW 先到、W 先到、AW/W 同时到达、`WSTRB`、寄存器读回、8 点脉冲响应、正/负饱和、配置上下限和限幅值反馈。

详细数据见 `verilog/iir/doc/test/ZYNQ7020_IIR_PL_VERIFICATION.md` 和
`verilog/iir/sim/*.csv`。

## 4. Vivado OOC 综合

目标为 `xc7z020clg400-2`，PL 时钟约束为 50MHz：

| 项目 | 结果 |
|------|------|
| WNS | +4.290ns |
| TNS | 0ns |
| LUT | 7625 |
| 寄存器 | 226 |
| DSP48E1 | 0 |
| 可操作 DRC | 0 |

OOC 报告包含一条预期的 `ZPS7-1`，表示独立 PL core 顶层没有 PS7。脚本只豁免该 Zynq 顶层规则，任何其他 DRC 都会失败。

## 5. 完整 PS+PL 实现

完整 block design 包含 PS7、DDR、AXI Interconnect、AXI GPIO、PL UART DMA、PL OLED DMA 和自定义 AXI IIR IP 2.0。

| 项目 | 结果 |
|------|------|
| 可操作 DRC | 0 |
| WNS | +0.272ns |
| TNS | 0ns |
| Slice LUT | 11317 / 53200，21.27% |
| Slice Register | 4075 / 106400，3.83% |
| DSP48E1 | 0 / 220，0% |
| bitstream | 已生成 |
| HDF | 已生成 |

## 6. ARM 无 RTOS 构建

当前无 RTOS Makefile 只定义 `IS_ZYNQ7020` 与 `TOOLCHAIN_GCC`，编译 `code/section/baremetal/section.c`，并优先包含同目录的 `section.h`。该工程不链接 SVC/IRQ 上下文切换端口；ELF 使用 Xilinx standalone 向量表 `_vector_table`。

板测版本记录的构建尺寸：

| text | data | bss | total |
|-----:|-----:|----:|------:|
| 144632 | 18008 | 40672 | 203312 |

链接段：

```text
.section_registry 0x00131EF0 size 0x460
.func_ram         0x00132350 size 0xF4
```

编译启用 `-Werror`、`-Wpedantic`、`-Wconversion`、`-Wsign-conversion`、`-Wshadow`、`-Wcast-align`、`-Wstrict-prototypes`、`-Wmissing-prototypes` 和 `-Wundef`，结果为 0 编译告警、0 错误。

## 7. ARM section SRTOS 构建

当前 A9 SRTOS Makefile 仍只定义 `IS_ZYNQ7020` 与 `TOOLCHAIN_GCC`，编译独立的 `code/section/srtos_a9/section.c`，并链接 `a9_section_control.c` 与 `a9_section_port.S`。A9 端口保存通用寄存器、VFP D0-D31、FPEXC、FPSCR、返回 PC 与 CPSR。

板测版本记录的构建尺寸：

| text | data | bss | total |
|-----:|-----:|----:|------:|
| 149528 | 18824 | 51040 | 219392 |

链接段：

```text
.section_registry 0x00138770 size 0x480
.func_ram         0x00138BF0 size 0xF4
```

严格编译结果为 0 编译告警、0 错误。A9 SRTOS ELF 包含 `a9_section_irq_handler` 与 `a9_section_port_yield`；裸机 ELF 不链接这两个 A9 端口符号。三套 section 头文件都保留统一的调度兼容函数声明，裸机实现对应空操作。

## 8. 板上联调结果

板载 PS UART1 使用 MIO48/MIO49，通过 CH340 映射为 COM6。baremetal 镜像下载后，PL AXI 自测结果为：

```text
DOWNLOAD_RESULT status=PASS srtos=0
PL_SELFTEST result=PASS gpio_input=0x00000001 gpio_output=0x00000005
iir_impulse=524288 131072 32768 8192 4294952960 512 1152 544
iir_saturation=0x7FFFFFFF/0x80000000
iir_limits=0x00000064/0xFFFFFFB5/0x00000064 iir_version=0x00020000
```

FRAME 通过 COM6 发送 `IIR_TEST` 与 `ZYNQ_STATUS`，目标端正确接收命令并返回：

```text
iir result=PASS impulse=524288,131072,32768,8192,-14336,512,1152,544
sat=2147483647/-2147483648 limits=100/-75 feedback=100 version=00020000
zynq mode=baremetal fault=0 save_fail=0 release_fail=0
BOARD_IIR_SELFTEST result=PASS port=COM6 baud=921600 mode=baremetal
```

随后下载 SRTOS 镜像。两次状态采样之间平台任务计数从 26 增长到 83，100ms 与 123ms 长任务均持续输出；公共现场池和公共运行栈状态正常：

```text
zynq mode=srtos-a9 tick_100us=81330 task_count=81 srtos=1
fault=0 save_fail=0 release_fail=0 pool=226/2048 stack_free=430

BOARD_IIR_SELFTEST result=PASS port=COM6 baud=921600 mode=srtos-a9
```

复测命令：

```powershell
cd D:\OneDrive\LWX\GD32\base\platform\zynq7020
.\ps\board_iir_selftest.ps1
.\ps\board_iir_selftest.ps1 -Srtos 1
```
