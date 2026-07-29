# 文档索引

本目录按工程职责归档设计、使用、移植、构建与验证资料。源码目录只保留参与编译的代码和模块内必要资源；项目级说明统一从本索引进入。

## 工程架构

- [工程设计](ENGINEERING_DESIGN.md)：仓库目录、公共软件层、平台层和验证工程的当前结构。
- [功能使用总览](guides/FEATURE_USAGE_GUIDE.md)：公共框架与调试功能的组合使用方式。

## 构建与移植

- [MCU 编译与下载](build/MCU_BUILD_DOWNLOAD_GUIDE.md)
- [PLECS 构建](build/PLECS_BUILD_GUIDE.md)
- [MCU 平台移植](porting/MCU_PLATFORM_PORTING.md)
- [Bootloader 平台移植](porting/BOOTLOADER_PLATFORM_PORTING.md)

## 框架

- Section：[设计](framework/section/SECTION_DESIGN.md) · [使用](framework/section/SECTION_USAGE.md)
- SRTOS：[设计](framework/srtos/SRTOS.md) · [测试用例](framework/srtos/SRTOS_TEST_CASES.md) · [测试报告](framework/srtos/reports/)

## 调试能力

- Perf：[设计](debug/perf/PERF_DESIGN.md) · [使用](debug/perf/PERF_USAGE.md)
- Scope：[设计](debug/scope/SCOPE_DESIGN.md) · [使用](debug/scope/SCOPE_USAGE.md)
- SFRA：[设计](debug/sfra/SFRA_DESIGN.md) · [使用](debug/sfra/SFRA_USAGE.md)
- Shell：[设计](debug/shell/SHELL_DESIGN.md) · [使用](debug/shell/SHELL_USAGE.md)
- Trace：[设计](debug/trace/TRACE_DESIGN.md) · [使用](debug/trace/TRACE_USAGE.md)

## 控制与算法

- [控制模块总设计](control/CTRL_DESIGN.md) · [通用接入](control/CTRL_USAGE.md)
- [时域仿真说明](control/CONTROL_TIME_DOMAIN_SIMULATIONS.md)
- 拓扑资料：[`bb/`](control/bb/) · [`boost/`](control/boost/) · [`buck/`](control/buck/) · [`cllc/`](control/cllc/) · [`inv/`](control/inv/) · [`llc/`](control/llc/) · [`pfc/`](control/pfc/) · [`pfc_i32/`](control/pfc_i32/)
- [PLL 参数整定](control/pll/PLL_PARAMETER_TUNING.md)

## 平台与验证

- Zynq-7020：[平台说明](platform/zynq7020/ZYNQ7020_PLATFORM.md) · [自测试报告](platform/zynq7020/ZYNQ7020_SELF_TEST_REPORT.md)
- GD32G553C：[厂商示例说明](platform/gd32g553c/VENDOR_EXAMPLE_README.txt)
- HC32F558：[平台说明](platform/hc32f558/PLATFORM_README.txt)
- 公共示例：[Section/调试功能演示](examples/DEMO.md)

## FPGA IP 文档

FPGA IP 的设计、应用和验证资料与 IP 源码共同维护：

- [`verilog/iir/doc/`](../verilog/iir/doc/)
- [`verilog/oled_dma/doc/`](../verilog/oled_dma/doc/)
- [`verilog/uart_dma/doc/`](../verilog/uart_dma/doc/)
