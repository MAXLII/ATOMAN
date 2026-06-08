# PFC 控制模块使用

通用接入流程见 [CTRL_USAGE.md](CTRL_USAGE.md)，内部设计见 [CTRL_PFC_DESIGN.md](CTRL_PFC_DESIGN.md)。

## 1. 配置

```c
pfc_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
};
pfc_cfg_set_timing(&timing);

pfc_cfg_set_vbus_ref_v(400.0f);
pfc_cfg_set_vbus_slew_vps(200.0f);
pfc_cfg_set_run_allowed(0u);
pfc_cfg_publish_building();
```

## 2. HAL 绑定

```c
pfc_hal_unlock_binding();
pfc_hal_set_v_g_ptr(&v_grid);
pfc_hal_set_v_rms_ptr(&v_grid_rms);
pfc_hal_set_i_l_ptr(&i_pfc_l);
pfc_hal_set_v_cap_ptr(&v_pfc_cap);
pfc_hal_set_v_bus_ptr(&v_bus);
pfc_hal_set_main_rly_is_closed_ptr(&main_rly_closed);
pfc_hal_set_vbus_sta_ptr(&vbus_sta);
pfc_hal_set_pwm_setter(pfc_pwm_set);
pfc_hal_set_pwm_enable(pfc_pwm_enable);
pfc_hal_set_pwm_disable(pfc_pwm_disable);
pfc_hal_set_main_rly_on_func(main_rly_on);
pfc_hal_set_main_rly_off_func(main_rly_off);
pfc_hal_set_latched_ptr(&pfc_latched);
pfc_hal_lock_binding();
```

## 3. 启停

```c
if (pfc_hal_is_ready() != 0u)
{
    pfc_fsm_set_cmd(pfc_fsm_cmd_start);
}

pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
```

## 4. 注意事项

- `pfc_ctrl_update_feedback()` 负责整理采样方向。
- PFC PWM setter 接收 `v_pwm` 和 `v_bus`。
- 主继电器闭合后 PFC 快速环路才进入运行门控。
- 母线参考和斜率使用 V、V/s。
