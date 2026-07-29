# SFRA 使用文档

## 1. 适用范围

本文档说明如何在当前工程中接入和使用 SFRA。

内部实现见 [SFRA_DESIGN.md](SFRA_DESIGN.md)。

## 2. 注册 SFRA 实例

引入头文件：

```c
#include "sfra.h"
```

使用 `REG_SFRA()` 注册实例：

```c
REG_SFRA(name, delay_tick, ts, inject_amp, freq_start, freq_end, prepare_cb, prepare_ctx)
```

参数说明：

| 参数 | 说明 |
| --- | --- |
| `name` | SFRA 实例名称 |
| `delay_tick` | 注入延迟 tick，用于对齐注入与采样 |
| `ts` | 控制 ISR 周期，单位 s |
| `inject_amp` | 注入幅值 |
| `freq_start` | 起始频率，单位 Hz |
| `freq_end` | 结束频率，单位 Hz |
| `prepare_cb` | 每个频点准备时的回调，可填 `NULL` |
| `prepare_ctx` | 传给回调的上下文 |

示例：

```c
REG_SFRA(demo_sfra,
         0u,
         50.0e-6f,
         0.01f,
         10.0f,
         1000.0f,
         NULL,
         NULL)
```

注册后会生成：

```c
demo_sfra
demo_sfra_inject
demo_sfra_collect
```

## 3. 控制环接入

在控制 ISR 的采样和控制计算位置接入：

```c
sfra_isr_pre_sample(&demo_sfra);

control_ref += demo_sfra_inject;

/* 控制计算与硬件输出 */

demo_sfra_collect = measured_response;
sfra_isr_post_sample(&demo_sfra);
```

在任务中周期调用：

```c
sfra_task(&demo_sfra);
```

`sfra_task()` 不需要和 ISR 同频，但需要被持续调用，用于推进扫频状态和计算结果。

## 4. 启停控制

代码中可直接调用：

```c
sfra_start(&demo_sfra);
sfra_stop(&demo_sfra);
sfra_reset(&demo_sfra);
```

设置扫频范围：

```c
sfra_set_sweep_range(&demo_sfra, 20.0f, 2000.0f);
```

设置注入延迟：

```c
sfra_set_inject_delay(&demo_sfra, 1u);
```

## 5. 上位机访问

SFRA 服务通过二进制协议提供访问能力：

1. 查询 SFRA 列表，获取 `sfra_id`。
2. 查询实例信息，确认扫频范围、注入幅值、状态和 `sweep_tag`。
3. 设置配置。
4. 启动扫频。
5. 等待完成上报，或主动查询频点。
6. 按 `point_index` 读取频率、幅值和相位。

二进制访问不依赖字符串 Shell。

## 6. 频点准备回调

如果每个频点开始前需要调整系统状态，可以提供回调：

```c
static void demo_sfra_prepare(void *p_ctx)
{
    (void)p_ctx;
    /* 切换量程、清状态或等待外部逻辑 */
}

REG_SFRA(demo_sfra,
         0u,
         50.0e-6f,
         0.01f,
         10.0f,
         1000.0f,
         demo_sfra_prepare,
         NULL)
```

## 7. 使用注意事项

- `ts` 必须等于实际调用 `sfra_isr_pre_sample/post_sample` 的周期。
- 注入幅值要从小幅值开始调试。
- `demo_sfra_collect` 应写入真实被测响应。
- SFRA 运行期间应避免外部逻辑频繁改变控制结构。
- 如果响应过小或注入过小，幅相计算可能不稳定。
- 扫频结果通过 `sweep_tag` 区分批次。

## 8. 注入延迟选择

`delay_tick` 用于对齐参与 DFT 计算的注入量和响应量。

判断方法：

| 对应关系 | delay_tick |
| --- | --- |
| `collect[n]` 是 `inject[n]` 的响应 | `0u` |
| `collect[n]` 是 `inject[n-1]` 的响应 | `1u` |
| `collect[n]` 是 `inject[n-2]` 的响应 | `2u` |

纯软件算法、无额外延迟的数学模型，通常使用 `0u`。如果控制器输出、PWM 更新、功率级或采样链路使当前注入到下一拍才反映到响应量，通常使用 `1u`。

如果扫频结果相位整体多出接近一个采样周期的滞后，检查 `delay_tick` 是否偏小。如果相位整体少了一个采样周期的滞后，检查 `delay_tick` 是否偏大。
