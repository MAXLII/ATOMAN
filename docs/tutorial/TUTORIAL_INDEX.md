# 教材总纲

教材用于理解嵌入式软件、控制原理、参数设计和仿真方法。内容以基础知识、原理推导、独立示例、MATLAB 脚本和结果图为主，可在接入具体工程前独立学习和验证。

## 嵌入式软件基础

- [C语言静态注册与链接基础](static_registration_and_linking.md)
- [C语言函数表与依赖倒置基础](c_function_table_and_dependency_inversion.md)
- [裸机调度、中断与状态机基础](baremetal_scheduling_interrupt_fsm.md)
- [嵌入式通信分发、性能与可靠性基础](communication_performance_reliability.md)
- [异步设备状态机基础](asynchronous_device_state_machine.md)
- [掉电安全持久化基础](power_fail_safe_persistence.md)
- [前后台数据一致性基础](foreground_background_data_consistency.md)
- [二进制序列化与兼容基础](binary_serialization_and_compatibility.md)

## 实时系统基础

- [实时任务与调度基础](realtime_task_scheduling.md)
- [处理器现场、栈与上下文切换基础](processor_context_and_stack.md)
- [RTOS并发、内存与可靠性基础](rtos_concurrency_and_reliability.md)

## 基础入口

- [控制原理时域仿真说明](control/control_time_domain_simulations.md)
- [PLL 参数整定](control/pll/pll_parameter_tuning.md)

## 拓扑与控制器

- BB：[原理时域仿真](control/bb/bb_principle_time_domain.m)
- Boost：[原理时域仿真](control/boost/boost_principle_time_domain.m)
- Buck：[原理时域仿真](control/buck/buck_principle_time_domain.m)
- INV：[原理时域仿真](control/inv/inv_principle_time_domain.m)
- LLC：[原理时域仿真](control/llc/llc_principle_time_domain.m)
- PFC：[浮点原理时域仿真](control/pfc/pfc_principle_time_domain.m) · [整数原理时域仿真](control/pfc_i32/pfc_i32_principle_time_domain.m)

## CLLC 专题

- [双竞争 PI MATLAB 教程](control/cllc/cllc_dual_compete_pi_matlab_tutorial.md)
- [2P2Z 控制器设计](control/cllc/cllc_2p2z_controller_design.m)
- [正向双 PI 设计](control/cllc/cllc_forward_dual_pi_design.m) · [时域仿真](control/cllc/cllc_forward_dual_pi_time_domain.m)
- [正向 PR 设计](control/cllc/cllc_forward_pr_design.m) · [PR/PI 纹波时域仿真](control/cllc/cllc_forward_pr_pi_ripple_time_domain.m)
- [频率增益分析](control/cllc/cllc_frequency_gain_analysis.m)
- [归一化混合 2P2Z 设计](control/cllc/cllc_hybrid_2p2z_normalized_design.m)
- [反向电压 PI 设计](control/cllc/cllc_reverse_voltage_pi_design.m) · [时域仿真](control/cllc/cllc_reverse_voltage_pi_time_domain.m)
