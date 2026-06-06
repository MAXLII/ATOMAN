# Scope 使用文档

## 1. 适用范围

本文档说明如何注册和使用 Scope 捕获调试波形。

内部实现见 [SCOPE_DESIGN.md](SCOPE_DESIGN.md)。

## 2. 注册 Scope

引入头文件：

```c
#include "scope.h"
```

注册一个默认 1000 us 采样周期的 Scope：

```c
REG_SCOPE(demo_scope, 256, 64, scope_vin, scope_iin, scope_vout)
```

注册一个指定采样周期的 Scope：

```c
REG_SCOPE_EX(ctrl_scope, 512, 128, 50u, scope_vbus, scope_iout)
```

参数说明：

| 参数 | 说明 |
| --- | --- |
| `name` | Scope 名称 |
| `buf_size` | 每个变量的样本数 |
| `trig_post_cnt` | 触发后继续采样的样本数 |
| `sample_period_us` | 采样周期，只有 `REG_SCOPE_EX()` 需要 |
| `var...` | 需要捕获的变量名 |

注册宏会自动创建 `float` 类型变量。业务代码直接给这些变量赋值。

## 3. 周期采样

在控制环或任务中更新 Scope 变量，然后调用 `SCOPE_RUN()`：

```c
scope_vin = adc_vin;
scope_iin = adc_iin;
scope_vout = ctrl_vout;

SCOPE_RUN(demo_scope);
```

`SCOPE_RUN()` 的调用周期应和注册时填写的 `sample_period_us` 一致，否则上位机显示的时间轴会不准确。

## 4. 启动和触发

手动启动：

```c
scope_start(&scope_demo_scope);
```

宏方式触发：

```c
SCOPE_TRIGGER(demo_scope);
```

典型用法：

```c
if (fault_edge_detected)
{
    SCOPE_TRIGGER(demo_scope);
}
```

触发后，Scope 会继续采集 `trig_post_cnt` 个样本，然后停止并置 `data_ready`。

## 5. 停止和复位

```c
scope_stop(&scope_demo_scope);
scope_reset(&scope_demo_scope);
```

`scope_reset()` 会清状态和缓冲。

## 6. 上位机访问

Scope 服务通过二进制协议提供访问能力：

1. 查询 Scope 列表，获取 `scope_id`。
2. 查询 Scope 信息，获取变量数量、样本数量、触发位置和 `capture_tag`。
3. 查询变量名。
4. 启动、触发、停止或复位。
5. 按样本索引读取波形数据。

上位机读取样本时应带上 `expected_capture_tag`，避免读取过程中 Scope 重新采样导致数据批次变化。

## 7. 可选 Shell 命令

如果打开 `SCOPE_ENABLE_PRINTF`，可以在注册 Scope 后追加：

```c
REG_SCOPE_STATUS_CMD(demo_scope)
REG_SCOPE_START_CMD(demo_scope)
REG_SCOPE_DATA_STEP_CMD(demo_scope)
```

默认这些宏关闭，不产生 Shell 命令。

## 8. 使用注意事项

- Scope 变量当前为 `float`。
- 注册变量数量最多 10 个。
- `buf_size` 越大，占用 RAM 越多。
- `trig_post_cnt` 必须小于或等于缓冲长度。
- 高频控制环中调用 `SCOPE_RUN()` 时，变量赋值应尽量简短。
- Scope 是调试工具，不应把控制逻辑依赖在 Scope 状态上。
