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
├─ gd32g553c/     GD32G553C 平台工程
├─ hc32f334/      HC32F334 平台工程
├─ hc32f558/      HC32F558 demo 平台工程
├─ apm32/         APM32 平台工程
├─ plecs/         PLECS 控制算法仿真工程
└─ docs/          工程设计、平台接入和专项设计文档
```

其中 `code/` 是公共代码目录。各 MCU 平台目录负责把公共代码接到具体硬件、官方库、编译工程和外设资源上；`plecs/` 只承接控制算法仿真所需的 `ctrl/` 与 `lib/`。

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
```

公共代码通过接口层访问硬件。平台变化时，优先修改平台 BSP 和 `interface/` 适配，不直接改控制算法主体。

## 相关文档

- 平台接入：[docs/MCU_PLATFORM_PORTING.md](docs/MCU_PLATFORM_PORTING.md)
- 工程设计：[docs/ENGINEERING_DESIGN.md](docs/ENGINEERING_DESIGN.md)
- 控制模块设计：[docs/CTRL_DESIGN.md](docs/CTRL_DESIGN.md)
- 控制模块使用：[docs/CTRL_USAGE.md](docs/CTRL_USAGE.md)

## 许可证

本仓库代码使用 MIT License，详见 [LICENSE](LICENSE)。

第三方组件，包括芯片厂商 SDK、官方外设库、Keil/JLink 相关文件和其他外部材料，不自动适用本仓库 MIT License。它们仍受各自原始许可证约束。
