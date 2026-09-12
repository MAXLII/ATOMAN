# NPC 正负序 dq 控制代码

## 幅值、相位及软启动配置

HAL 通过 `npc_hal_set_vd_pos_ref_ptr()` 挂载正序 d 轴电压幅值指针，通过 `npc_hal_set_theta_ptr()` 挂载相位指针。控制器每拍读取两者；幅值不再由 cfg/FSM 发布。幅值单位 V、范围 0～1000000；相位保持 0～2π rad，Shell 上限仍为 6.283186。

cfg 的 `vd_pos_slew_vps` 配置软启动及参考升降速率，使用 `npc_cfg_set_vd_pos_slew_vps()` 提交，由 FSM 发布。允许 0.001～1000000 V/s，默认约 281.69132 V/s，额定幅值约 2 s 升起。每拍爬升量为该速率乘控制周期，过渡时间为幅值差除以速率。上位机新增 `VD_POS_SLEW`；原 `VD_POS_REF`、幅值别名和 `THETA` 保留。

## 架构与职责

NPC 以当前 `code/ctrl/buck` 的配置请求、FSM 发布快照、HAL 指针绑定和注册控制中断为参照；三相浮点采样与外部相位输入按 NPC 拓扑定义。原先由 app 持有控制实例并逐拍调用 `init/reset/cal` 的接口已撤销。

| 文件 | 持有的数据与职责 | 主要接口/执行入口 |
|---|---|---|
| `npc_hal.h/.c` | 静态采样指针、PWM 回调、运行进入/退出回调、保护闭锁 | `get_ctrl/get_fsm`、分项绑定 setter、绑定锁、`hard_protect_*` |
| `npc_cfg.h/.c` | 应用参数、运行请求、未发布 setpoint、采样周期、固定候选控制系数 | 参数 setter、`get_run_request/get_p_building`、`get_run_state`、`npc_cfg_default` |
| `npc_fsm.h/.c` | init/idle/run 状态和唯一的已发布 setpoint | `REG_FSM`（1 ms）、`get_run_sta`、`read_published` |
| `npc_ctrl.h/.c` | 固定地址 DSOGI、八个 PI 积分器、参考爬升、控制监视量 | `REG_INIT`、`REG_INTERRUPT`（5 kHz）、`prepare_run/stop/get_monitor` |
| `platform/plecs/npc/app/app.c` | DLL 采样缓存、外部相位发生器、Shell 暂存与监视副本、时间网格、日志 | HAL 绑定、cfg setter、`section_interrupt()`、`run_task()` |
| `code/interface/npc/common/pwm.c` | SVPWM 调制与每相正/负占空比生成 | `pwm_init/update/disable` |
| `platform/plecs/npc/bsp/bsp_pwm.c` | 六路占空比和一个使能的宿主输出适配 | 写入 DLL 输出帧 |

### 结构体约定

- `npc_ctrl_hal_t`：挂载三相输出电压、三相电感电流、上下半母线、当前相位的 **float 指针**；挂载 `p_set_pwm_func` 和 `p_pwm_disable`。发波函数接收当前 αβ 电压指令及上下母线，返回 0 表示成功，非零保留调制故障码。HAL 不包含 `p_init`、控制器实例、配置构造器或 PWM 实现。
- `npc_fsm_hal_t`：只挂载运行进入和退出动作。默认进入调用 `npc_ctrl_prepare_run()`，默认退出调用 `npc_ctrl_stop()`。不会仅因进入 run 就置高 PWM 使能；必须有一次有效控制计算和调制输出。
- `npc_cfg_t`：应用物理参数及运行请求。`npc_ctrl_setpoint_t`：给控制中断使用的参数及 FSM 授予的 `run_allowed`。应用通过 cfg setter 提交，不能直接改发布对象或运行许可。
- `npc_fsm_published_t`：FSM 私有的原子快照。发布时先置奇数版本、写字段、再置偶数版本；ISR 仅尝试一次读取，碰到正在发布则保留前一完整快照，不自旋等待。
- `npc_ctrl_cfg_t`：固定系数集合；保留当前 MATLAB 候选 PI、限流和调制裕量。初始化复制到内部固定地址状态，不能从 app 修改内部 DSOGI 绑定。
- `npc_ctrl_input_t`、`npc_ctrl_inter_t`、`npc_ctrl_t`：仅在 `npc_ctrl.c` 内定义。分别负责同拍采样快照、观测器与积分状态、算法实例，不作为跨层接口暴露。
- `npc_ctrl_output_t`、`npc_ctrl_monitor_t`：只读诊断结果，包括 dq 反馈、给定、限幅状态、积分监视副本及实际爬升参考。app 在控制结束后复制给 Shell/CSV，不参与控制计算。

### 执行和所有权

1. 平台停止通信调度，解锁 HAL，绑定所有模拟量指针及发波/关断回调，再锁定。配置控制周期并初始化平台 PWM，最后调用 `section_init()`。
2. cfg 的注册初始化恢复默认参数和关闭请求；FSM 初始化恢复 init 状态和关闭许可；ctrl 初始化建立固定地址的 DSOGI 绑定。支持同一个已加载 DLL 多次开始仿真。
3. 平台每 200 μs 更新模拟采样，提交 Shell 参数与运行请求。外部相位使用 FSM 已发布的频率，和控制器 DSOGI 保持一致。随后只调用 `section_interrupt()` 分派控制。
4. ctrl 中断读取 HAL 指针、检测原始采样保护、读取 FSM 快照、检查运行许可和母线条件，再执行软启动、DSOGI 和八 PI，调用 HAL 发波函数。首拍给定为零，成功输出后才推进爬升。
5. `run_task()` 驱动 1 ms FSM。init 等待 HAL/cfg 就绪；idle 接受运行请求并确认无闭锁；退出 idle 锁住绑定；进入 run 调用准备回调，run 持续发布参数；停机或保护使其关闭许可并调用退出回调，回到 idle 后允许重绑定。
6. 停机请求由控制中断直接检查，最迟下一次 200 μs 更新关断，不等待 1 ms FSM；保护也在当拍关闭 PWM。故障只在请求关闭且采样恢复健康后确认，不能因波形回落自行重启。

cfg 参数在下一次 1 ms FSM 发布后生效；HAL 幅值和相位每拍读取；启动要经过 init/idle/run。这里的任务响应时间与 PWM 一拍流水延迟不同：**DLL 内没有额外缓存上一拍电压指令，当前计算结果直接送到 PWM**，模型自行处理调制延迟和死区。

PLECS 通过现有 dispatch lock 串行化 Shell、初始化、FSM 和控制中断；绑定、准备/停止和监视副本读取均需遵守这一串行约定。移植到真实中断平台时，平台需提供相应的安全调用时机，不能把本次 DLL 编译当作 MCU 中断并发或 RAM 链接验证。

## 接入接口

平台已有完整绑定示例见 `platform/plecs/npc/app/app.c` 的 `bind_control_hal()`。每路分别使用 `npc_hal_set_v_out_ptr(phase, p_value)` 和 `npc_hal_set_i_l_ptr(phase, p_value)`；phase 为 A=0、B=1、C=2。母线和相位使用各自指针 setter。

启动仿真后设置 `FREQ_HZ=50`、`VD_POS_REF=563.38264`、`RUN_ENABLE=1`。应用对应调用 `npc_cfg_set_freq_hz()`、`npc_cfg_set_vd_pos_slew_vps()`、`npc_cfg_set_run_request()`；不直接进入 FSM 状态、不直接改 `run_allowed`、不逐拍调用算法私有函数。

输出电压按相电压 V 输入，电流按 A 输入，桥臂流向负载为正；上下母线均为正幅值。相位为同拍 rad，控制器不包含 PLL。参考为正序 d 轴相电压峰值，690 V 线电压 RMS 对应约 563.38264 V。dq 各数组顺序为 `[d+,q+,d-,q-]`。

## 控制律与验证边界

控制结构对应 `docs/images/npc_sequence_dq_control.svg`：电压、电流分别经 Clarke、DSOGI、正负序 Park；四个电压 PI 产生四轴电流给定，四个电流 PI 产生电压，经反 Park 合成 αβ。正序 q 轴及负序电压给定为零。

此次保留原有数学实现：当前积分输出参与计算，联合限幅后更新八个积分器；电流给定按正负序模长之和限制，电压按重构三相最大最小值差限制，采用实际统一缩放结果回算。PI 参数、电流给定上限 5200 A、过流阈值 6240 A、过压阈值 1000 V、2 s 额定爬升保持。

`npc_ctrl_isr` 和内部 `npc_ctrl_cal` 使用 `FUNC_RAM`；目标数学库、DSOGI/SOGI 和其他被调函数的最终放置仍需 MCU 链接核对。

验证：MinGW Release DLL 严格警告编译通过；静态比较完整数学核心（只忽略内部函数更名和 static 链接属性）、41 个 Shell 注册项、CSV 采集函数和 DLL 时间网格通过。未运行 testbench、MATLAB 或 PLECS。历史 `platform/testbench/npc` 基于已撤销的实例式 API，不能用于当前架构验收，需另行按 HAL/Section 契约迁移；本次不改该历史验证工程。

## PLECS 运行诊断

### 5 kHz 原始控制采样 CSV

重新加载带日志的 DLL 后，启动仿真会自动置 `TRACE_ENABLE=1` 等待开机。收到 `RUN_ENABLE=1` 后开始连续记录 100000 拍（20 s 仿真时间），每拍 200 us。文件位于实际加载的 DLL 同目录，名称为 `npc_trace_UTC日期_时间_毫秒_进程号_序号.csv`，每次独立创建，不覆盖旧文件。CSV 的 `time_s` 是仿真时间，文件名时间是 UTC 主机时间。

`TRACE_STATE`：0 空闲、1 等待开机、2 正在写入、3 已完成、4 文件读写失败。`TRACE_ROWS` 是当前采集已写入的行数。完成后 `TRACE_ENABLE` 自动变为 0；再次写 1 可在运行中抓取另一个 20 s 窗口。写 0 可提前结束并关闭文件，重新触发前让 DLL 至少处理一次该停止请求。采集中即使发生保护故障也继续记录至窗口结束，便于观察停机后过程；仿真终止会关闭文件。

每行包含同一控制采样的母线、三相电压电流、相位、给定、正负序 dq、四路电流给定、alpha/beta 计算和 PWM 指令、八个 PI 积分状态、限幅标志、六路占空比、使能及状态码。记录发生在 `update()` 后、Shell 任务前；故障拍的控制内部量可能已复位，故障前的完整轨迹和原始模拟输入仍保留。占空比是 DLL 输出，不是 `.plecs` 延迟和死区之后的门极信号。

使用缓冲写入，每 5000 行刷新一次，避免每拍刷盘；仍会增加主机仿真耗时，但不修改仿真时间步。该日志不包含 5 kHz 控制采样之间的开关纹波。文件 I/O 失败会停止采集并报告状态 4，不改变控制和保护设置。运行日志新增 `NPC APP build=...` 及实际 PI 参数、DLL 内部延迟拍数，避免仅看未重编译的日志适配文件时间而误判版本。

此次架构重写完成 DLL 编译及静态契约核对，未运行控制仿真。

日志版本在实际加载的 DLL 同目录追加写入 `npc_runtime.log`，启动行含编译日期、时间和进程号；`LOG_READY=1` 表示文件成功打开。原有 stderr 输出仍保留。状态或失败细节变化时才记录，记录包括仿真时间、调用计数、运行请求、幅值、频率、两段母线、门限和电压指令。

上位机读取 `CTRL_TICKS` 判断 5 kHz 更新是否发生：仿真时间每前进 1 s 约增加 5000，不以主机墙钟时间计数。`CTRL_STATUS` 是当前状态，`CTRL_DETAIL` 是相关细节：

| 状态值 | 含义 |
|---|---|
| 0 | 初始化中（保留启动状态） |
| 1 | 运行请求关闭，或等待 FSM 授予运行许可 |
| 2 | 控制计算与 SVPWM 均成功 |
| 3 | 幅值、频率或 DSOGI 配置被拒绝 |
| 4 | `V_DC_HALF_MIN` 配置被拒绝 |
| 5 | 输入为非有限数或超 float 范围，detail 为从 0 开始的 DLL 输入序号 |
| 6 | 半母线低于门限，读取 `V_DC_P`、`V_DC_N` |
| 7 | 控制器计算拒绝，需检查幅值、相位、模拟采样与数值范围 |
| 8 | SVPWM 拒绝，detail 保留错误枚举；`NOT_READY=0` 映射为 `UINT32_MAX`，避免与 HAL 成功值冲突 |
| 9 | 时间异常；detail=0 启动时间非法，1 非法时间/回退，2 漏掉周期 |
| 10 | 初始化或端口异常；detail=0 初始化失败，1 回调端口/初始化不可用 |
| 11 | 仿真已终止 |
| 12 | 实际电感电流越限闭锁，detail 为相序号 A=0、B=1、C=2 |

启动回调会将 `RUN_ENABLE` 和 `VD_POS_REF` 置 0；需在仿真启动后下发。状态 2 只代表控制链执行成功，若幅值仍为 0，则不会主动建立目标输出电压。重新启动仿真以加载新 DLL。

## Y–Δ 参数落地与启动保护

`npc_cfg_default()` 与 `npc_yd_design_config.m` 已同步包含动态调制误差的联合设计参数：电压环 Kp=1、Ki=1300；电流环 Kp=0.15、Ki=0.05；正负序 D/Q 使用相同的对应环路增益。电流给定联合限制 5200 A，采样周期 200 μs，回算增益 10，调制裕量 0.95。此次优先改善实测低频振荡的稳定裕度，大负载阶跃响应有所放慢。参数通过所测模型的稳定性检查，但实际 PLECS 波形及卸载过压尚未通过验收；设计依据与限制见 [NPC_OSCILLATION_PI_REDESIGN.md](NPC_OSCILLATION_PI_REDESIGN.md)。

控制器对 HAL 读取的 `VD_POS_REF` 按 FSM 发布的 `vd_pos_slew_vps` 限制变化速度，默认约 281.69132 V/s，额定 563.38264 V 从零升起需要约 2 s，对应 690 V 线电压 RMS。首个有效控制采样使用零给定，后续成功更新推进爬升；其他幅值的爬升时间按幅值变化量成比例变化。`VD_POS_REF_ACT` 是本拍实际送入控制器的给定。停机或控制链失败清零爬升；频率改变也会重新爬升。

每拍在 DSOGI 和 PI 前检测原始三相电感电流的绝对值。过流门限为 6240 A（电流给定限制的 1.2 倍），`TRIP_CURRENT` 是其 Shell 监视副本。使能时过流，立即关闭本拍 PWM 输出并清空控制动态，保留首次故障状态与相序号；日志记录触发时三相电压、电流及电流门限。故障不会随采样恢复自行重新发波。

按卸载仿真调试要求，NPC 输出过压停机逻辑已移除，不再产生状态 13，Shell 不再注册 `TRIP_VOLTAGE`。启动日志包含 `overvoltage_trip=off`。母线欠压、过流、非有限输入及计算有效性检查仍保留；电压采样继续用于反馈控制。

PLECS 的 Shell 参数均可写，注册状态不包含只读标记，注册上下限保持不同。写入仍按上下限处理；采样值、计算结果和状态监视副本会由后续仿真更新覆盖，写入监视副本不等同于改变控制器内部状态。修改后需重新启动仿真加载 DLL，并在上位机重新读取参数列表。此修改不涉及 MCU Shell 或 Section 链表顺序。

后续注册顺序修复：PLECS GCC 注册记录使用 `no_reorder` 保持同文件声明顺序，跨文件沿用链接输入顺序。NPC 使用的 baremetal 后端按注册顺序遍历就绪任务；初始化和中断均按优先级数值升序执行，同优先级保持注册顺序。三个 Section 后端的中断同优先级插入均已改为稳定排序，这也会影响采用这些后端的 MCU 固件；不同优先级规则不变。该优先级是框架回调顺序，不是硬件中断控制器的抢占优先级。

顺序回归测试 `npc_section_order_test` 链接实际 NPC/Section 对象，在独立测试进程中屏蔽业务回调，仅执行顺序标记。运行 `cmake --build platform/plecs/npc/build --target npc_section_order_test`，再运行 `ctest --test-dir platform/plecs/npc/build -R "^npc_section_order$" --output-on-failure`，可检查混合及同优先级、就绪任务跳过、重复初始化和实际回调执行顺序。该测试不启动仿真或网络，也不验证 MCU 的中断抢占与 SRTOS 调度。

上位机操作：仿真启动后设 `FREQ_HZ=50`、`VD_POS_REF=563.38264`，再设 `RUN_ENABLE=1`。过流故障后先排除原因，在全部模拟采样有限且三相电流回到门限内时保持 `RUN_ENABLE=0` 至少一个控制采样，确认 `CTRL_STATUS=1` 后再写 `RUN_ENABLE=1`。重新启动从零爬升。

这是 5 kHz 采样保护，不能限制采样间峰值；骤卸负载的储能过压仍未解决，关断后续流和能量泄放需要在实际电路模型中处理。MATLAB 稳定性结果不等同于当前 C/DLL 仿真验证。此次已重新编译 DLL，未运行 testbench 或 PLECS，也未修改 `.plecs` 文件。
