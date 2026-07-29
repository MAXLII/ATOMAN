# Buck-Boost 控制模块使用

通用接入流程见 [ctrl_usage.md](../ctrl_usage.md)，内部设计见 [ctrl_bb_design.md](../../../design/control/bb/ctrl_bb_design.md)。

## 1. 配置

```c
bb_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
};
bb_cfg_set_timing(&timing);

bb_cfg_set_out_volt_ref(48.0f);
bb_cfg_set_pwr_lmt(1000.0f);
bb_cfg_set_in_volt_lmt(20.0f);
bb_cfg_set_in_curr_lmt(50.0f);
bb_cfg_set_out_curr_lmt(50.0f);
bb_cfg_set_run_allowed(0u);
bb_cfg_publish_building();
```

## 2. HAL 绑定

```c
bb_hal_unlock_binding();
bb_hal_set_v_in_ptr(&v_in);
bb_hal_set_i_in_ptr(&i_in);
bb_hal_set_v_out_ptr(&v_out);
bb_hal_set_i_out_ptr(&i_out);
bb_hal_set_i_l_ptr(&i_l);
bb_hal_set_pwm_setter(bb_pwm_set);
bb_hal_set_pwm_disable(bb_pwm_disable);
bb_hal_set_latched_ptr(&bb_latched);
bb_hal_lock_binding();
```

## 3. 启停

```c
if (bb_hal_is_ready() != 0u)
{
    bb_fsm_set_cmd(bb_fsm_cmd_start);
}

bb_fsm_set_cmd(bb_fsm_cmd_stop);
```

## 4. 注意事项

- `bb_ctrl_update_feedback()` 负责整理输入/输出电压电流和电感电流采样。
- BB PWM setter 同时处理 buck 和 boost 两组 duty 与上下管使能。
- `bb_ctrl_in_curr_lmt_task()` 以 1 ms 周期把输入功率限制换算成输入电流限制。

## 5. 拓扑移植重点

- 输入、输出和电感电流统一使用当前控制器约定的物理单位与功率流向。
- 实际 `ctrl_ts`、PWM 周期和 1 ms 功率限制任务必须与控制参数一致。
- 根据硬件电感重新核对开环模型、环路系数和电流限幅。
- Buck、Buck-Boost、Boost 三个区域的进入/退出阈值必须形成回差。
- CCM 与 DCM 使用相同的模式语义，切换时不得产生 duty 或桥臂使能跳变。
- 输入电压、输入电流和输出电流三个限制环的最小值作为外环上限。
- 输入功率限制换算满足 `in_curr_lmt = min(pwr_lmt / v_in, configured_lmt)`，并处理低输入电压。

## 6. 拓扑验收

1. 分别验证 Buck、Buck-Boost 和 Boost 三个工作区间。
2. 验证 CCM/DCM 进入和退出回差，不在边界来回抖动。
3. 逐一触发输入电压、输入电流、输出电流和功率限制。
4. 检查电感电流参考的方向、限幅和启动过程。
5. stop 与保护触发后，Buck 和 Boost 两组桥臂都进入安全状态。

## 7. 关联导航

- 源码：[BB 控制](../../../../code/ctrl/bb/bb_ctrl.c) · [BB HAL](../../../../code/ctrl/bb/bb_hal.c) · [BB FSM](../../../../code/ctrl/bb/bb_fsm.c)
- 设计：[BB 控制设计](../../../design/control/bb/ctrl_bb_design.md) · [控制 HAL 挂载与生命周期设计](../../../design/control/hal_binding_lifecycle_design.md)
