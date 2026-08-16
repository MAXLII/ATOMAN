# 设计文档总纲

设计文档说明仓库、公共软件、控制模块、调试模块、框架、平台和 FPGA IP 的当前结构与内部职责。阅读时先了解整体工程，再进入具体模块。

## 阅读顺序

1. [工程设计](../engineering_design.md)：仓库目录、软件分层、平台工程、Flash 升级架构和验证结构。
2. 模块总设计：掌握一类公共能力的接口、数据流和内部组成。
3. 具体模块设计：查看单个控制拓扑、调试功能或框架的实现约束。
4. 平台与 FPGA IP 设计：查看目标平台或逻辑 IP 的专用结构。

## 控制模块

- [控制模块总设计](control/ctrl_design.md)
- [控制 HAL 挂载与生命周期设计](control/hal_binding_lifecycle_design.md)
- [控制参数构建与发布设计](control/setpoint_publish_design.md)
- BB：[运行模式设计](control/bb/bb_mode_design.md) · [控制设计](control/bb/ctrl_bb_design.md)
- Boost：[控制设计](control/boost/ctrl_boost_design.md)
- Buck：[整数控制设计](control/buck/buck_integer_control_design.md) · [控制设计](control/buck/ctrl_buck_design.md)
- CLLC：[控制设计](control/cllc/ctrl_cllc_design.md)
- INV：[整型控制设计](control/inv/inv_integer_control_design.md) · [控制设计](control/inv/ctrl_inv_design.md)
- PFC：[控制设计](control/pfc/ctrl_pfc_design.md)

## 框架与调试

- [公共软件组件模型](framework/component_model.md)
- [调试与观测系统总设计](debug/debug_system_design.md)
- [故障现场诊断设计](debug/fault_diagnosis_design.md)
- Section：[总体设计](framework/section/section_design.md) · [Link](framework/section/link_design.md) · [FSM](framework/section/fsm_design.md) · [Perf 协作](framework/section/perf_design.md)
- SRTOS：[设计](framework/srtos/srtos.md)
- Perf：[设计](debug/perf/perf_design.md)
- Scope：[设计](debug/scope/scope_design.md)
- SFRA：[设计](debug/sfra/sfra_design.md)
- Shell：[设计](debug/shell/shell_design.md)
- Trace：[设计](debug/trace/trace_design.md)

## 通信、存储与升级

- [FRAME通信核心设计](communication/frame_design.md)
- [协议演进与兼容设计](communication/protocol_evolution_design.md)
- [FAL分区与异步Flash管理设计](storage/fal_design.md)
- [Bootloader升级与防变砖设计](bootloader/bootloader_design.md)

## 平台

- Zynq-7020：[平台设计](platform/zynq7020/zynq7020_platform.md)

## FPGA IP

FPGA IP 的设计文档与对应 RTL 共同维护：

- [IIR 设计文档](../../verilog/iir/doc/design/axi_iir_3p3z_design.md)
- [OLED DMA 设计文档](../../verilog/oled_dma/doc/design/zynq7020_pl_oled_dma_design.md)
- [UART DMA 设计文档](../../verilog/uart_dma/doc/design/zynq7020_pl_uart_dma_design.md)
