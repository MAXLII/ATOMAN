# Buck 控制模块使用

通用接入流程见 [CTRL_USAGE.md](CTRL_USAGE.md)，内部设计见 [CTRL_BUCK_DESIGN.md](CTRL_BUCK_DESIGN.md)。

## 1. 配置

```c
buck_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
    .task_ts = 100.0e-6f,
    .pwm_cmp_max = 3000,
};
buck_cfg_set_timing(&timing);

buck_cfg_set_out_volt_ref(12.0f);
buck_cfg_set_in_volt_lmt(24.0f);
buck_cfg_set_pwr_lmt(1000.0f);
buck_cfg_set_in_curr_lmt(50.0f);
buck_cfg_set_out_curr_lmt(50.0f);
buck_cfg_set_run_allowed(0u);
buck_cfg_publish_building();
```

## 2. HAL 绑定

```c
buck_hal_unlock_binding();
buck_hal_set_v_in_ptr(&v_in_code);
buck_hal_set_i_in_ptr(&i_in_code);
buck_hal_set_v_out_ptr(&v_out_code);
buck_hal_set_i_out_ptr(&i_out_code);
buck_hal_set_i_l_ptr(0u, &i_l0_code);
buck_hal_set_i_l_ptr(1u, &i_l1_code);
buck_hal_set_pwm_setter(0u, buck_pwm0_set);
buck_hal_set_pwm_setter(1u, buck_pwm1_set);
buck_hal_set_pwm_disable(buck_pwm_disable);
buck_hal_set_latched_ptr(&buck_latched);
buck_hal_lock_binding();
```

## 3. 启停

```c
if (buck_hal_is_ready() != 0u)
{
    buck_fsm_set_cmd(buck_fsm_cmd_start);
}

buck_fsm_set_cmd(buck_fsm_cmd_stop);
```

## 4. 注意事项

- `buck_cfg_set_*()` 接收物理量，内部保存为整数代码值。
- HAL 采样指针类型为 `int32_t *`。
- 电感电流通道号小于 `BUCK_CTRL_IND_CURR_CH_NUM`。
- PWM setter 输出 compare 和上下管使能。
