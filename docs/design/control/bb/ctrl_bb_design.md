# Buck-Boost 控制模块设计

## 1. 模块定位

BB 模块位于 `code/ctrl/bb/`，用于宽范围 Buck-Boost 功率级控制。该模块使用浮点物理量控制域。

## 2. 拓扑特征

- 控制域使用浮点物理量，setpoint同时保存输出参考、功率和输入/输出限制。
- 1ms任务把功率限制换算为输入电流限制，优先级3的ISR完成限制环、电压环、电流环和调制。
- 一个PWM setter同时接收Buck与Boost两组duty和上下管使能，保证模式切换作为一次输出提交。
- CCM闭合电感电流环，DCM复位电流环并使用开环duty生成器，两条路径共享模式语义。

公共Cfg、HAL、Ctrl和FSM分层见[控制模块总设计](../ctrl_design.md)。

## 3. 控制框图

```mermaid
flowchart LR
    cfg["active setpoint<br/>Vout ref / limits / run_allowed"]
    sample["反馈采样整理<br/>bb_ctrl_update_feedback"]
    slow["1 ms 输入电流限制<br/>pwr_lmt / v_in"]
    invlmt["输入电压限制 PI"]
    iinlmt["输入电流限制 PI"]
    ioutlmt["输出电流限制 PI"]
    minlmt["最小限制选择<br/>outer loop upper limit"]
    vloop["输出电压 PI<br/>out_volt_loop"]
    iref["电感电流参考"]
    iloop["电感电流 PI<br/>ind_curr_loop"]
    mode["Buck / Buck-Boost / Boost 模式选择"]
    ccm["CCM duty 合成<br/>buck: (v_l + v_out) / v_in<br/>boost: (v_in - v_l) / v_out"]
    dcm["DCM 开环 duty 合成"]
    pwm["HAL PWM setter<br/>buck duty / boost duty"]

    cfg --> slow --> iinlmt
    sample --> slow
    sample --> invlmt
    sample --> iinlmt
    sample --> ioutlmt
    invlmt --> minlmt
    iinlmt --> minlmt
    ioutlmt --> minlmt
    cfg --> vloop
    sample --> vloop
    minlmt --> vloop --> iref
    iref --> iloop
    sample --> iloop
    sample --> mode
    sample --> ccm
    sample --> dcm
    mode --> ccm
    mode --> dcm
    iloop --> ccm --> pwm
    vloop --> dcm --> pwm
```

BB ISR 整理 HAL 采样，计算 CCM/DCM 状态、限制环、输出电压环和电感电流环。CCM 路径闭合电感电流环后合成 duty；DCM 路径复位电流环并使用开环 duty 生成器。

## 4. 约束

- 三个工作区与CCM/DCM切换阈值必须形成回差。
- 联合PWM setter必须一次提交两组桥臂的duty和使能状态。

## 5. 关联导航

- 应用：[BB控制使用](../../../application/control/bb/ctrl_bb_usage.md)
- 公共设计：[控制模块总设计](../ctrl_design.md) · [控制HAL挂载与生命周期设计](../hal_binding_lifecycle_design.md)
