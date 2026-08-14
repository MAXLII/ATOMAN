# CTRL 控制模块设计总览

## 1. 模块定位

`code/ctrl/` 存放硬件无关的功率级控制模块。当前包含：

| 目录 | 模块 | 控制域 |
| --- | --- | --- |
| `code/ctrl/pfc/` | 交流侧 PFC 控制 | 浮点物理量 |
| `code/ctrl/inv/` | 逆变输出控制 | 浮点物理量 |
| `code/ctrl/bb/` | Buck-Boost 控制 | 浮点物理量 |
| `code/ctrl/cllc/` | 双向 CLLC 控制 | 浮点物理量 |
| `code/ctrl/buck/` | Buck 控制 | 整数代码域 |
| `code/ctrl/boost/` | Boost 控制 | 整数代码域 |

控制模块不直接访问 MCU 寄存器。平台采样、PWM 输出、继电器和保护动作通过 HAL 绑定进入控制模块。

## 2. 通用分层

每个控制模块按四层组织：

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 配置层 | `*_cfg.c/h`、`*_cfg_fsm.h` | timing、setpoint、active/building 双缓冲、物理量到代码域转换和 FSM 私有发布接口 |
| HAL 层 | `*_hal.c/h` | 采样指针、PWM 回调、保护回调、运行进入/退出回调 |
| 控制层 | `*_ctrl.c/h` | 初始化、运行准备、反馈采样整理、ISR 控制、慢速任务 |
| FSM 层 | `*_fsm.c/h` | init、idle、run 相关状态和 start/stop 命令 |

控制层通过 Section 注册到统一调度入口。典型注册包括：

| 注册 | 作用 |
| --- | --- |
| `REG_INIT()` | 初始化控制对象 |
| `REG_INTERRUPT()` | 控制 ISR 阶段执行快速环路 |
| `REG_TASK()` / `REG_TASK_MS()` | 慢速计算、参数发布、辅助状态更新 |
| `REG_FSM()` | 状态机任务 |

## 3. 配置双缓冲

控制设定值使用 active/building 双缓冲：

| 缓冲 | 用途 |
| --- | --- |
| `building` | 上层写入配置 |
| `active` | 控制侧读取配置 |

`*_cfg_set_*()` 写入 building buffer。应用发送 start 命令后，FSM 通过私有 `*_cfg_fsm.h` 接口设置 `run_allowed` 并发布完整配置。控制侧通过 `*_cfg_sync_building_to_active()` 或 fast sync 同步到 active。

Buck 和 Boost 的配置接口接收物理量，内部保存为整数代码域。PFC、INV 和 BB 保存浮点物理量。

## 4. HAL 与采样整理

HAL 层绑定平台提供的采样变量和回调函数。控制 ISR 不直接表达平台拓扑差异，而是在控制层入口整理采样：

| 模块 | 采样整理 |
| --- | --- |
| PFC | `pfc_ctrl_update_feedback()` 整理 `v_g`、`v_cap`、`i_l`、`v_bus`、`v_rms` 和主继电器反馈 |
| INV | `inv_ctrl_update_feedback()` 整理 `v_cap`、`i_l`、`v_bus` |
| BB | `bb_ctrl_update_feedback()` 整理 `v_in`、`i_in`、`v_out`、`i_out`、`i_l` |
| CLLC | `cllc_ctrl` 整理 `v_battery`、`i_battery`、`v_bus`，按 FSM 锁存方向选择正向或反向控制集 |
| Boost | `update_adc_feedback()` 整理 `v_in_fb`、`v_out_fb`、`i_l_fb[]` |
| Buck | ISR 和任务直接读取整数代码域采样 |

PFC 在 PLECS 中与 INV 复用拓扑产生的电感电流方向适配放在 `pfc_ctrl_update_feedback()` 内。控制框图只体现整理后的控制方向。

## 5. 运行入口

平台接入控制模块时完成以下动作：

1. 初始化平台硬件并保持 PWM 关闭。
2. 配置 timing，写入 setpoint building buffer。
3. 调用 `section_init()`，由主循环推进 FSM 进入 Idle。
4. 在 Idle 解锁 HAL，绑定采样指针、PWM 回调、保护回调和状态机资源。
5. 通过 `*_hal_is_ready()` 验证后锁定 HAL。
6. 主循环持续调用 `run_task()`。
7. 控制 ISR 中调用 `section_interrupt()`。
8. 通过模块 FSM 命令启动或停止；FSM 统一管理发布、运行许可和 HAL 生命周期调用顺序。

进入 run 时，FSM 调用 `*_ctrl_prepare_run()` 重新初始化控制状态。

## 6. 文档索引

控制模块与平台之间的依赖冻结规则见[控制 HAL 挂载与生命周期设计](hal_binding_lifecycle_design.md)，平台接入见[控制 HAL 平台挂载方法](../../application/control/hal_binding_usage.md)。控制参数从后台构建到实时路径生效的并发边界见[控制参数构建与发布设计](setpoint_publish_design.md)，接入顺序见[控制参数发布使用方法](../../application/control/setpoint_publish_usage.md)。

| 模块 | 设计文档 | 使用文档 |
| --- | --- | --- |
| PFC | [ctrl_pfc_design.md](pfc/ctrl_pfc_design.md) | [ctrl_pfc_usage.md](../../application/control/pfc/ctrl_pfc_usage.md) |
| INV | [ctrl_inv_design.md](inv/ctrl_inv_design.md) | [ctrl_inv_usage.md](../../application/control/inv/ctrl_inv_usage.md) |
| BB | [ctrl_bb_design.md](bb/ctrl_bb_design.md) | [ctrl_bb_usage.md](../../application/control/bb/ctrl_bb_usage.md) |
| CLLC | [ctrl_cllc_design.md](cllc/ctrl_cllc_design.md) | [ctrl_cllc_usage.md](../../application/control/cllc/ctrl_cllc_usage.md) |
| Buck | [ctrl_buck_design.md](buck/ctrl_buck_design.md) | [ctrl_buck_usage.md](../../application/control/buck/ctrl_buck_usage.md) |
| Boost | [ctrl_boost_design.md](boost/ctrl_boost_design.md) | [ctrl_boost_usage.md](../../application/control/boost/ctrl_boost_usage.md) |

## 7. 关联导航

- 应用：[控制模块通用接入](../../application/control/ctrl_usage.md) · [控制 HAL 平台挂载方法](../../application/control/hal_binding_usage.md) · [控制参数发布使用方法](../../application/control/setpoint_publish_usage.md)
- 基础教材：[前后台数据一致性基础](../../tutorial/foreground_background_data_consistency.md) · [控制原理时域仿真说明](../../tutorial/control/control_time_domain_simulations.md)
