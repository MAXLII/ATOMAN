# INV 控制模块使用

通用接入流程见 [CTRL_USAGE.md](CTRL_USAGE.md)，内部设计见 [CTRL_INV_DESIGN.md](CTRL_INV_DESIGN.md)。

## 1. 配置

```c
inv_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
    .ctrl_freq = 20000.0f,
};
inv_cfg_set_timing(&timing);

inv_cfg_set_freq_hz(50.0f);
inv_cfg_set_freq_slew_hzps(10.0f);
inv_cfg_set_rms_ref_v(230.0f);
inv_cfg_set_rms_slew_vps(212.0f);
inv_cfg_set_run_allowed(0u);
inv_cfg_publish_building();
```

## 2. HAL 绑定

```c
inv_hal_unlock_binding();
inv_hal_set_v_cap_ptr(&v_inv_cap);
inv_hal_set_i_l_ptr(&i_inv_l);
inv_hal_set_v_bus_ptr(&v_bus);
inv_hal_set_pwm_setter(inv_pwm_set);
inv_hal_set_pwm_enable(inv_pwm_enable);
inv_hal_set_pwm_disable(inv_pwm_disable);
inv_hal_set_inv_rly_on_func(inv_rly_on);
inv_hal_set_inv_rly_off_func(inv_rly_off);
inv_hal_set_latched_ptr(&inv_latched);
inv_hal_lock_binding();
```

## 3. 启停

```c
if (inv_hal_is_ready() != 0u)
{
    inv_fsm_set_cmd(inv_fsm_cmd_start);
}

inv_fsm_set_cmd(inv_fsm_cmd_stop);
```

## 4. 注意事项

- `inv_ctrl_update_feedback()` 负责整理电压、电流和母线采样。
- INV PWM setter 接收 `v_pwm` 和 `v_bus`。
- 频率和电压参考按配置斜率变化。
- 采样更新需要先于 INV 控制 ISR。
