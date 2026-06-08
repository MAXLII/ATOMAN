# Boost 控制模块使用

通用接入流程见 [CTRL_USAGE.md](CTRL_USAGE.md)，内部设计见 [CTRL_BOOST_DESIGN.md](CTRL_BOOST_DESIGN.md)。

## 1. 配置

```c
boost_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
    .task_ts = 100.0e-6f,
    .pwm_ts = 50.0e-6f,
    .pwm_cmp_max = 3000,
};
boost_cfg_set_timing(&timing);

boost_cfg_set_out_volt_ref(400.0f);
boost_cfg_set_in_volt_lmt(24.0f);
boost_cfg_set_pwr_lmt(1000.0f);
boost_cfg_set_in_curr_lmt(50.0f);
boost_cfg_set_out_curr_lmt(50.0f);
boost_cfg_set_run_allowed(0u);
boost_cfg_publish_building();
```

## 2. HAL 绑定

```c
boost_hal_unlock_binding();
boost_hal_set_v_in_ptr(&v_in_code);
boost_hal_set_v_out_ptr(&v_out_code);
boost_hal_set_i_l_ptr(0u, &i_l0_code);
boost_hal_set_i_l_ptr(1u, &i_l1_code);
boost_hal_set_pwm_setter(0u, boost_pwm0_set);
boost_hal_set_pwm_setter(1u, boost_pwm1_set);
boost_hal_set_pwm_enable(boost_pwm_enable);
boost_hal_set_pwm_disable(boost_pwm_disable);
boost_hal_set_latched_ptr(&boost_latched);
boost_hal_lock_binding();
```

## 3. 启停

```c
if (boost_hal_is_ready() != 0u)
{
    boost_fsm_set_cmd(boost_fsm_cmd_start);
}

boost_fsm_set_cmd(boost_fsm_cmd_stop);
```

## 4. 注意事项

- `boost_cfg_set_*()` 接收物理量，内部保存为整数代码值。
- `update_adc_feedback()` 负责整理 Boost 控制反馈。
- 电感电流通道号小于 `BOOST_CTRL_IND_CURR_CH_NUM`。
- PWM setter 输出 compare 和上下管使能。
