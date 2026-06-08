# CTRL 控制模块设计总览

## 1. 模块定位

`code/ctrl/` 存放硬件无关的功率级控制模块。当前包含：

| 目录 | 模块 | 控制域 |
| --- | --- | --- |
| `code/ctrl/pfc/` | 交流侧 PFC 控制 | 浮点物理量 |
| `code/ctrl/inv/` | 逆变输出控制 | 浮点物理量 |
| `code/ctrl/bb/` | Buck-Boost 控制 | 浮点物理量 |
| `code/ctrl/buck/` | Buck 控制 | 整数代码域 |
| `code/ctrl/boost/` | Boost 控制 | 整数代码域 |

控制模块不直接访问 MCU 寄存器。平台采样、PWM 输出、继电器和保护动作通过 HAL 绑定进入控制模块。

## 2. 通用分层

每个控制模块按四层组织：

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 配置层 | `*_cfg.c/h` | timing、setpoint、active/building 双缓冲、物理量到代码域转换 |
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

`*_cfg_set_*()` 写入 building buffer。`*_cfg_publish_building()` 发布配置。控制侧通过 `*_cfg_sync_building_to_active()` 或 fast sync 同步到 active。

Buck 和 Boost 的配置接口接收物理量，内部保存为整数代码域。PFC、INV 和 BB 保存浮点物理量。

## 4. HAL 与采样整理

HAL 层绑定平台提供的采样变量和回调函数。控制 ISR 不直接表达平台拓扑差异，而是在控制层入口整理采样：

| 模块 | 采样整理 |
| --- | --- |
| PFC | `pfc_ctrl_update_feedback()` 整理 `v_g`、`v_cap`、`i_l`、`v_bus`、`v_rms` 和主继电器反馈 |
| INV | `inv_ctrl_update_feedback()` 整理 `v_cap`、`i_l`、`v_bus` |
| BB | `bb_ctrl_update_feedback()` 整理 `v_in`、`i_in`、`v_out`、`i_out`、`i_l` |
| Boost | `update_adc_feedback()` 整理 `v_in_fb`、`v_out_fb`、`i_l_fb[]` |
| Buck | ISR 和任务直接读取整数代码域采样 |

PFC 在 PLECS 中与 INV 复用拓扑产生的电感电流方向适配放在 `pfc_ctrl_update_feedback()` 内。控制框图只体现整理后的控制方向。

## 5. 运行入口

平台接入控制模块时完成以下动作：

1. 配置 timing。
2. 写入 setpoint building buffer 并 publish。
3. 解锁 HAL，绑定采样指针、PWM 回调、保护回调和状态机回调。
4. 锁定 HAL。
5. 调用 `section_init()`。
6. 主循环持续调用 `run_task()`。
7. 控制 ISR 中调用 `section_interrupt()`。
8. 通过模块 FSM 命令启动或停止。

进入 run 时，FSM 调用 `*_ctrl_prepare_run()` 重新初始化控制状态。

## 6. 文档索引

| 模块 | 设计文档 | 使用文档 |
| --- | --- | --- |
| PFC | [CTRL_PFC_DESIGN.md](CTRL_PFC_DESIGN.md) | [CTRL_PFC_USAGE.md](CTRL_PFC_USAGE.md) |
| INV | [CTRL_INV_DESIGN.md](CTRL_INV_DESIGN.md) | [CTRL_INV_USAGE.md](CTRL_INV_USAGE.md) |
| BB | [CTRL_BB_DESIGN.md](CTRL_BB_DESIGN.md) | [CTRL_BB_USAGE.md](CTRL_BB_USAGE.md) |
| Buck | [CTRL_BUCK_DESIGN.md](CTRL_BUCK_DESIGN.md) | [CTRL_BUCK_USAGE.md](CTRL_BUCK_USAGE.md) |
| Boost | [CTRL_BOOST_DESIGN.md](CTRL_BOOST_DESIGN.md) | [CTRL_BOOST_USAGE.md](CTRL_BOOST_USAGE.md) |
