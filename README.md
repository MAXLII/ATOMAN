# 数字电源 demo 工程

本仓库是一个面向数字电源开发的 demo 工程，用来验证公共代码、MCU 平台工程和 PLECS 仿真之间的组织方式。

这个 demo 主要分为三部分：

1. 自动注册机制：通过 `code/section` 对需要注册服务的对象做自动收集、统一编排和调度入口生成。
2. 部分 MCU 平台工程：用官方 SDK、官方 demo 工程和规格书，把公共代码接入到不同 MCU 平台的 `interface/` 中。
3. PLECS 仿真：只仿真 `ctrl/` 以及 `ctrl/` 调用到的 `lib/`，不耦合 `code/` 内其他业务、通信、调试和平台相关逻辑。

它不是某个具体产品板的完整固件。平台接入、工程设计和具体模块说明放在 `docs/` 中维护。

## 当前包含什么

```text
base
├─ code/          自动注册、控制、通信、调试、接口和基础算法代码
├─ platform/      MCU、MATLAB 和 PLECS 平台工程
│  ├─ apm32/      APM32 平台工程
│  ├─ gd32g553c/  GD32G553C 平台工程
│  ├─ hc32f334/   HC32F334 平台工程
│  ├─ hc32f558/   HC32F558 demo 平台工程
│  ├─ zynq7020/   Zynq-7020 Cortex-A9 + PL 平台工程
│  │  ├─ ps/      ARM 软件、BSP、构建、下载和板测工程
│  │  └─ pl/      Vivado 硬件平台、IP、约束和 PL 自测工程
│  ├─ matlab/     MATLAB 仿真与分析工程
│  └─ plecs/      PLECS 控制算法仿真工程
└─ docs/          工程设计、平台接入和专项设计文档
```

其中 `code/` 是公共代码目录。`platform/` 下的 MCU 平台负责把公共代码接到具体硬件、官方库、编译工程和外设资源上；`platform/plecs/` 承接控制算法仿真所需的 `ctrl/` 与 `lib/`。

HC32F334 平台同时提供 `gcc/` 和 `keil_mdk/` 两套工程。两套工程各自包含 `compile.bat` 和 `download.bat`；GCC 固件下载通过 Keil 的 HC32F334 Flash 算法烧录 GCC 生成的 HEX，成功条件包含 Flash 校验通过。

## `code/` 分层

```text
code
├─ app/          上层流程、保护、demo、升级和业务逻辑
├─ comm/         基础通信协议、CRC、命令解析和路由
├─ ctrl/         PFC、INV、Buck、Boost、Buck-Boost 等控制模块
├─ dbg/          perf、scope、trace、shell、SFRA 等调试观测能力
├─ interface/    ADC、PWM、GPIO、USART 等平台接口封装
├─ lib/          PI、滤波、SOGI、DFT、RMS 等基础算法
└─ section/      自动注册、任务调度、中断调度和链路调度框架
   ├─ baremetal/ 裸机 section.c/.h
   ├─ srtos_m/   Cortex-M SRTOS section.c/.h
   └─ srtos_a9/  Cortex-A9 SRTOS section.c/.h
```

Section 构建目标只选择一个运行时目录，并让该目录中的 `section.h` 位于公共 `code/section/` 之前。三套实现使用相同注册接口，不通过 `SRTOS` 构建宏切换。公共代码通过接口层访问硬件。平台变化时，优先修改平台 BSP 和 `interface/` 适配，不直接改控制算法主体。

## 相关文档

- 功能使用接入指南：[docs/FEATURE_USAGE_GUIDE.md](docs/FEATURE_USAGE_GUIDE.md)
- 平台接入：[docs/MCU_PLATFORM_PORTING.md](docs/MCU_PLATFORM_PORTING.md)
- MCU 编译与下载：[docs/MCU_BUILD_DOWNLOAD_GUIDE.md](docs/MCU_BUILD_DOWNLOAD_GUIDE.md)
- Zynq-7020 平台：[docs/ZYNQ7020_PLATFORM.md](docs/ZYNQ7020_PLATFORM.md)
- Zynq-7020 IIR PL 验证：[docs/ZYNQ7020_IIR_PL_VERIFICATION.md](docs/ZYNQ7020_IIR_PL_VERIFICATION.md)
- Zynq-7020 自测报告：[docs/ZYNQ7020_SELF_TEST_REPORT.md](docs/ZYNQ7020_SELF_TEST_REPORT.md)
- 工程设计：[docs/ENGINEERING_DESIGN.md](docs/ENGINEERING_DESIGN.md)
- 控制模块设计：[docs/CTRL_DESIGN.md](docs/CTRL_DESIGN.md)
- 控制模块使用：[docs/CTRL_USAGE.md](docs/CTRL_USAGE.md)

## 许可证

本仓库代码使用 MIT License，详见 [LICENSE](LICENSE)。

第三方组件，包括芯片厂商 SDK、官方外设库、Keil/JLink 相关文件和其他外部材料，不自动适用本仓库 MIT License。它们仍受各自原始许可证约束。
