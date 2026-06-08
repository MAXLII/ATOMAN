# Buck-Boost 控制模块使用

通用接入流程见 [CTRL_USAGE.md](CTRL_USAGE.md)，内部设计见 [CTRL_BB_DESIGN.md](CTRL_BB_DESIGN.md)。

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
