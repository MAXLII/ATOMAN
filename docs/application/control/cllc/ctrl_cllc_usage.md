# CLLC 双向控制模块使用

通用接入流程见 [ctrl_usage.md](../ctrl_usage.md)，内部设计和 PI 公式见 [ctrl_cllc_design.md](../../../design/control/cllc/ctrl_cllc_design.md)。

## 1. 设置 timing 和参考值

```c
cllc_ctrl_timing_t timing = {
    .ctrl_ts = 10.0e-6f,
    .task_ts = 1.0e-3f,
    .startup_delay_ticks = 1u,
};

cllc_cfg_set_timing(&timing);
cllc_cfg_set_battery_voltage_ref(48.0f);
cllc_cfg_set_battery_current_limit(165.0f);
cllc_cfg_set_bus_voltage_ref(450.0f);
cllc_cfg_set_direction(CLLC_DIRECTION_FORWARD);
```

`ctrl_ts` 是正向共享积分 PI 和反向 Tustin PI 的实际调用周期。`task_ts` 是 FSM 的调用周期，用于把默认启动延时换算为 tick。

## 2. HAL 绑定

```c
cllc_hal_unlock_binding();
cllc_hal_set_v_battery_ptr(&v_battery);
cllc_hal_set_i_battery_ptr(&i_battery);
cllc_hal_set_v_bus_ptr(&v_bus);
cllc_hal_set_modulation_setter(cllc_pwm_set_normalized);
cllc_hal_set_pwm_enable(cllc_pwm_enable_direction);
cllc_hal_set_pwm_disable(cllc_pwm_disable_all);
cllc_hal_set_latched_ptr(&cllc_fault_latched);
cllc_hal_lock_binding();
```

调制回调接口为：

```c
void cllc_pwm_set_normalized(CLLC_DIRECTION_E direction, float u);
```

平台根据 `direction` 选择参与工作的桥臂，并把 `u` 映射为对应方向的 PSM/PFM。`u` 始终限制在 `0~1`。

## 3. 正向启动

```c
cllc_cfg_set_direction(CLLC_DIRECTION_FORWARD);
cllc_cfg_set_battery_voltage_ref(48.0f);
cllc_cfg_set_battery_current_limit(165.0f);
cllc_fsm_set_cmd(CLLC_FSM_CMD_START);
```

正向状态下，电池电压环负责稳压，电池电流限制环在过流时通过最小值竞争接管输出。

## 4. 反向启动

```c
cllc_cfg_set_direction(CLLC_DIRECTION_REVERSE);
cllc_cfg_set_bus_voltage_ref(450.0f);
cllc_fsm_set_cmd(CLLC_FSM_CMD_START);
```

反向状态下只执行母线电压 PI。母线参考 setter 自动限制在 400~500 V。

## 5. 停机和换向

运行中不能直接换向。完整换向顺序为：

```c
cllc_fsm_set_cmd(CLLC_FSM_CMD_STOP);

if (cllc_fsm_get_run_state() == CLLC_RUN_STATE_IDLE)
{
    cllc_cfg_set_direction(CLLC_DIRECTION_REVERSE);
    cllc_fsm_set_cmd(CLLC_FSM_CMD_START);
}
```

`cllc_cfg_set_direction()` 只在 FSM 处于 idle 时接受新方向。startup、run 和 fault 状态下调用该接口时，新方向会被忽略，building setpoint 与实际桥臂方向均保持不变。应用完成参数修改后只发送 start 命令，FSM 在接受启动时统一发布配置并锁存方向。`direction_mismatch` 保留为配置缓冲区被外部直接改写时的防御性诊断量。

## 6. 保护与复位

```c
cllc_hal_hard_protect_trip();

if (cllc_fsm_get_run_state() == CLLC_RUN_STATE_FAULT)
{
    cllc_fsm_set_cmd(CLLC_FSM_CMD_RESET);
}
```

硬保护首先立即关闭 PWM，再由 1 ms FSM 任务进入 fault。reset 清除锁存并返回 idle，不会自动重新启动。

## 7. 调试量

```c
cllc_ctrl_debug_t debug = {0};
cllc_ctrl_get_debug(&debug);
```

`debug` 包含当前锁存方向、电压参考与反馈、电流限制参考与反馈、最终归一化输出、正向两路候选值、100 Hz PR 输出、电流限制接管标志和运行中方向不一致标志。PLECS 的 `PLECS_OUTPUT_DBG` 输出正向 PR 的-0.5~0.5双极性补偿结果。

## 8. PLECS 工程

`platform/plecs/cllc/` 通过 `code/interface/cllc/adc.c` 和 `pwm.c` 接入本模块。编译命令：

```bat
cd platform\plecs\cllc
compile.bat
```

输出文件为 `platform/plecs/cllc/build/bin/libplecs.dll`。DLL 使用10路输入和40路输出，具体顺序由 `platform/plecs/cllc/plecs_port.h` 定义。前6路输出分别为原边和副边的 `ENABLE / DUTY / FREQUENCY_HZ`：正向运行时仅原边桥输出有效，反向运行时仅副边桥输出有效，停机时两侧全部清零。控制器的归一化命令由 `PLECS_OUTPUT_PI_OUTPUT` 继续提供。

## 9. 关联导航

- 源码：[CLLC 控制](../../../../code/ctrl/cllc/cllc_ctrl.c) · [CLLC HAL](../../../../code/ctrl/cllc/cllc_hal.c) · [CLLC FSM](../../../../code/ctrl/cllc/cllc_fsm.c) · [CLLC PLECS 入口](../../../../platform/plecs/cllc/app/app.c)
- 设计：[CLLC 控制设计](../../../design/control/cllc/ctrl_cllc_design.md) · [控制 HAL 挂载与生命周期设计](../../../design/control/hal_binding_lifecycle_design.md)
