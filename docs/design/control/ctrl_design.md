# CTRL 控制模块组织架构

## 1. 模块定位

`code/ctrl/` 按功率拓扑组织控制代码。控制器通过 HAL 获取采样和调用发波接口，复用 `code/lib/` 算法，不直接操作 MCU 寄存器。

| 目录 | 用途 | 主要控制域 |
|---|---|---|
| `buck/`、`boost/` | 降压、升压控制 | 整数代码域 |
| `bb/` | Buck-Boost 模式与控制 | 浮点物理量 |
| `llc/`、`cllc/` | 谐振与双向谐振控制 | 浮点物理量 |
| `pfc/`、`pfc_i32/` | PFC 控制 | 浮点／整数实现 |
| `inv/`、`inv_dq/`、`inv_i32/` | 逆变输出控制 | 浮点、DQ 与整数实现 |
| `npc/` | 三电平 NPC 正负序控制 | 浮点物理量 |

## 2. 四类文件的职责

```text
应用参数 ── cfg ── 待发布参数 ── fsm ── 完整参数与运行许可
                                  │                 │
                            生命周期动作           ▼
平台采样 ── HAL 指针 ── 本拍采样 ── 保护 ── ctrl 算法并发波
                                             │          │
                                        code/app    HAL 回调
                                                        │
                                                Interface → BSP
```

| 文件 | 负责什么 |
|---|---|
| `*_cfg.c/.h` | 运行请求、软起动参数、控制配置及待发布参数；不持有算法积分历史 |
| `*_hal.c/.h` | 挂载模拟采样指针、必要的实时幅值/相位、发波停波和生命周期回调 |
| `*_fsm.c/.h` | 初始化、待机、运行及拓扑特有启动时序，决定参数提交与运行许可 |
| `*_ctrl.c/.h` | 采样快照、斜坡和算法状态、控制计算、限幅及发波 |

保护判据和恢复策略属于 `code/app/`；控制层提供停止、抑制执行等机制。Interface 适配控制输出的语义，BSP 负责具体硬件或仿真端口。

## 3. 参数与实时数据

应用提出运行请求，FSM 根据生命周期条件授予运行许可，两者分开。参数先构建、再完整发布，控制中断只消费一致的快照。

现有模块有两种实现：部分使用 cfg 的 active/building 缓冲，提交接口统一声明在 `*_cfg.h` 并标注仅供 FSM 调用；Buck、NPC 由 FSM 发布并提供有界快照读取。两者的共同目标是明确写入者和生效时刻，不让控制器读到半新半旧参数。

HAL 保存长期有效的采样地址和动作回调。实时幅值或相位如果通过 HAL 输入，就不再增加第二个独立可写来源；软起动配置放 cfg，实际斜坡状态放 ctrl。

## 4. 生命周期与执行顺序

新拓扑采用以下顺序：

1. 平台保持输出关闭，在 INIT 阶段完成绑定和配置。
2. INIT 检查依赖与初始化结果，通过后锁定绑定并进入 IDLE。
3. FSM 响应启动请求，完成必要时序、准备算法状态并发布运行许可。
4. 每拍依次执行采样、应用保护、控制计算与发波。
5. 停止或故障时撤销执行许可并停止输出；需要重绑时返回初始化流程。

状态函数按进入、执行、转移检查、退出组织。PFC 可包含软启动和继电器等待，LLC/CLLC 可包含桥臂启动等待，双向拓扑还需要明确方向切换边界。

Section 通过 `REG_INIT`、`REG_INTERRUPT`、`REG_TASK`/`REG_TASK_MS` 和 `REG_FSM` 组织执行。NPC 已分别注册采样、应用保护、控制发波三阶段；其他历史模块仍存在合并 ISR 和不同优先级，不能假定已全部统一。具体任务周期和中断顺序由目标工程核对。

## 5. 新增拓扑

沿用 cfg/HAL/FSM/ctrl 的职责与数据流，根据实际拓扑替换采样量、控制计算、调制接口和必要状态，不照搬其他功率级的系数。详细固定格式见[仓库控制拓扑技能](../../../.agents/skills/base-ctrl-topology/SKILL.md)。

本页说明总体组织；函数和结构体规范、并发发布机制及平台挂载细节分别在技能和下列文档中展开。

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
