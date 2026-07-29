# Boost 控制模块设计

## 1. 模块定位

Boost 模块位于 `code/ctrl/boost/`，用于升压功率级控制。该模块使用整数代码域，配置层把物理量转换成控制代码。

## 2. 拓扑特征

- `ctrl_ts`、`task_ts`、`pwm_ts` 和 `pwm_cmp_max`共同定义整数控制与调制时基。
- 输入、输出电压和多通道电感电流均使用代码域，通道数由`BOOST_CTRL_IND_CURR_CH_NUM`确定。
- 慢速任务计算功率、电压和电流限制，优先级3的控制ISR执行多相电流环及PWM输出。
- PWM输出在CCM compare和DCM duty之间带回差切换，平台同时提供分通道setter及整体enable/disable。

公共Cfg、HAL、Ctrl和FSM分层见[控制模块总设计](../ctrl_design.md)。

## 3. 控制框图

```mermaid
flowchart LR
    cfg["active setpoint<br/>integer code domain"]
    slow["慢速任务<br/>boost_task"]
    vin["HAL: v_in code"]
    vout["HAL: v_out code"]
    il["HAL: i_l[ch] code"]
    sample["反馈采样整理<br/>v_in_fb / v_out_fb / i_l_fb"]
    pwr["功率限制<br/>pwr_lmt / v_in"]
    invlmt["输入电压限制 PI"]
    currlmt["输入/输出电流限制换算"]
    minlmt["电感电流上限选择<br/>i_l_lmt"]
    pending["ISR pending 参数<br/>i_l_lmt"]
    vloop["输出电压 PI<br/>out_volt_loop"]
    split["多相电流参考分配"]
    iloop["每通道电感电流 PI"]
    ff["输入电压前馈<br/>v_in * K4"]
    ccm["CCM compare<br/>(vin_ff - PI) / v_out"]
    dcm["DCM duty 修正"]
    mode["CCM/DCM 选择与滞回"]
    pwm["HAL PWM setter[ch]"]

    cfg --> slow
    vin --> slow
    vout --> slow
    slow --> pwr --> currlmt
    slow --> invlmt --> minlmt
    currlmt --> minlmt --> pending
    vin --> sample
    vout --> sample
    il --> sample
    pending --> vloop
    cfg --> vloop
    sample --> vloop --> split --> iloop
    sample --> iloop
    sample --> ff --> ccm
    iloop --> ccm
    sample --> ccm
    ccm --> mode
    ccm --> dcm --> mode
    mode --> pwm
```

Boost ISR 整理 ADC 反馈，执行输出电压环和每通道电感电流环，再根据 CCM compare 和 DCM duty 结果做模式滞回选择。

## 4. 约束

- 平台提供正确的 `pwm_ts` 和 `pwm_cmp_max`。
- 多通道电感电流和 PWM setter 按通道绑定。
- CCM/DCM模式切换不得改变通道身份或产生compare跳变。

## 5. 关联导航

- 应用：[Boost控制使用](../../../application/control/boost/ctrl_boost_usage.md)
- 公共设计：[控制模块总设计](../ctrl_design.md) · [控制HAL挂载与生命周期设计](../hal_binding_lifecycle_design.md)
