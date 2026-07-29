# 应用文档总纲

应用文档说明公共模块如何接入目标工程，以及工程如何编译、下载、移植和运行。可先阅读功能总览，再按实际目标选择构建、移植或模块操作文档。

## 综合入口

- [功能使用总览](guides/feature_usage_guide.md)
- [公共功能演示](examples/demo.md)

## 构建与移植

- [MCU 编译与下载](build/mcu_build_download_guide.md)
- [PLECS 构建](build/plecs_build_guide.md)
- [MCU 平台移植](porting/mcu_platform_porting.md)
- [Bootloader 平台移植](porting/bootloader_platform_porting.md)

## 通信、存储与升级

- [FRAME通信接入](communication/frame_usage.md)
- [通信命令开发方法](communication/command_development_usage.md)
- [FAL平台配置与上层接入](storage/fal_usage.md)
- [Bootloader升级接入与运行](bootloader/bootloader_upgrade_usage.md)

## 框架与调试

- [公共组件接入方法](framework/component_integration.md)
- Section：[使用](framework/section/section_usage.md)
- SRTOS：[使用](framework/srtos/srtos_usage.md) · [M 系列接入](framework/srtos/srtos_m_porting.md) · [A 系列接入](framework/srtos/srtos_a_porting.md)
- Perf：[使用](debug/perf/perf_usage.md)
- Scope：[使用](debug/scope/scope_usage.md)
- SFRA：[使用](debug/sfra/sfra_usage.md)
- Shell：[使用](debug/shell/shell_usage.md)
- Trace：[使用](debug/trace/trace_usage.md)
- [故障现场诊断使用方法](debug/fault_diagnosis_usage.md)

## 控制模块

- [控制模块通用接入](control/ctrl_usage.md)
- [控制参数发布使用方法](control/setpoint_publish_usage.md)
- BB：[运行模式使用](control/bb/bb_mode_usage.md) · [控制使用](control/bb/ctrl_bb_usage.md) · [移植检查](control/bb/porting_checklist.md)
- Boost：[控制使用](control/boost/ctrl_boost_usage.md)
- Buck：[控制使用](control/buck/ctrl_buck_usage.md)
- CLLC：[控制使用](control/cllc/ctrl_cllc_usage.md)
- INV：[控制使用](control/inv/ctrl_inv_usage.md) · [移植检查](control/inv/porting_checklist.md)
- PFC：[控制使用](control/pfc/ctrl_pfc_usage.md) · [移植检查](control/pfc/porting_checklist.md)

## 算法库

- [控制算法库使用方法](library/control_blocks_usage.md)
- [信号处理算法库使用方法](library/signal_processing_usage.md)
- [检测与时序算法库使用方法](library/detection_sequence_usage.md)

## FPGA IP

FPGA IP 的应用文档与对应 RTL 共同维护：

- [IIR 应用文档](../../verilog/iir/doc/application/)
- [OLED DMA 应用文档](../../verilog/oled_dma/doc/application/)
- [UART DMA 应用文档](../../verilog/uart_dma/doc/application/)
