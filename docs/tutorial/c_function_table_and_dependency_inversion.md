# C语言函数表与依赖倒置基础

## 1. 学习目标

本文讨论如何在C语言中使用函数指针表隔离变化，并说明上下文指针、对象所有权、适配器和单例封装之间的关系。内容只讨论通用软件知识，不依赖特定工程。

学习后应能够判断：

- 为什么直接调用硬件函数会扩大依赖；
- 函数表与普通回调有什么区别；
- 何时需要`void *context`；
- 何时可以把平台单例隐藏在适配器中；
- 哪些“抽象层”只是无意义转发。

## 2. 编译依赖的方向

假设一个记录模块直接调用器件函数：

```c
void record_save(const uint8_t *data, uint32_t length)
{
    w25q64_sector_erase(0x1000u);
    w25q64_page_program(0x1000u, data, length);
}
```

记录策略因此知道器件型号、地址和擦写命令。更换存储器或在主机上测试时，记录模块也要修改。

依赖倒置不是消除依赖，而是让稳定规则依赖一个较小的能力契约：

```c
typedef struct
{
    int (*erase)(uint32_t offset, uint32_t length);
    int (*write)(uint32_t offset,
                 const uint8_t *data,
                 uint32_t length);
} storage_ops_t;
```

高层仍然依赖“可擦、可写”，但不再依赖某个器件。

## 3. 函数表是一组能力

单个回调通常表达一次事件：

```c
typedef void (*complete_callback_t)(int result);
```

函数表表达一个协作对象能够提供的多项能力：

```c
typedef struct
{
    int (*read)(uint32_t offset, uint8_t *data, uint32_t length);
    int (*write)(uint32_t offset, const uint8_t *data, uint32_t length);
    int (*erase)(uint32_t offset, uint32_t length);
    int (*state_get)(void);
} storage_ops_t;
```

函数表在初始化时整体挂载，使模块可以一次验证契约是否完整。运行时不应反复猜测某个必需函数是否存在。

## 4. 上下文指针

如果一个驱动可能有多个实例，函数需要知道操作对象：

```c
typedef struct
{
    void *context;
    int (*read)(void *context,
                uint32_t offset,
                uint8_t *data,
                uint32_t length);
} storage_ops_t;
```

挂载时：

```c
static spi_flash_t flash_a;

static const storage_ops_t ops = {
    .context = &flash_a,
    .read = spi_flash_read_adapter,
};
```

`void *`允许接口不暴露具体类型，但牺牲了编译期类型检查。适配函数应尽早转换并检查：

```c
static int spi_flash_read_adapter(void *context,
                                  uint32_t offset,
                                  uint8_t *data,
                                  uint32_t length)
{
    spi_flash_t *flash = context;

    if (flash == NULL)
    {
        return -1;
    }
    return spi_flash_read(flash, offset, data, length);
}
```

## 5. 单例与显式上下文

如果一个平台确定只有一个存储实例，可以对上隐藏上下文：

```c
static int platform_read(uint32_t offset,
                         uint8_t *data,
                         uint32_t length)
{
    return spi_flash_read(&g_platform_flash, offset, data, length);
}
```

这减少了上层参数，但把“只能有一个实例”的选择放入平台适配器。通用核心是否保留上下文，应由它自身的复用需求决定，不能因为某个平台只有一个实例，就把所有公共代码都改成全局单例。

判断原则：

- 多实例、测试隔离或库复用需要显式上下文；
- 平台固定单例可以由最外层适配器封装；
- 不要在两三层接口中重复传递永远相同、且上层无权使用的内部对象。

## 6. 所有权与使用权

函数表只授予调用能力，不自动转移对象所有权。

例如模块得到`read/write`函数后，并不意味着它可以：

- 重新初始化底层设备；
- 修改底层配置；
- 停止其他使用者共享的状态机；
- 释放底层上下文；
- 绕过分区和权限。

接口设计应只暴露使用者职责范围内的函数。所有权越清晰，初始化顺序和错误恢复越容易判断。

## 7. 配置与运行状态分离

```c
typedef struct
{
    uint32_t capacity;
    uint32_t block_size;
} module_cfg_t;

typedef struct
{
    const module_cfg_t *cfg;
    const storage_ops_t *ops;
    uint32_t progress;
    uint8_t state;
} module_t;
```

配置通常在编译目标中固定，可以是`const`。进度和状态随运行变化，属于实例上下文。分离二者可以防止状态机意外修改静态事实，也便于多个实例共享只读配置。

## 8. 适配器的作用

适配器有价值的情况包括：

- 枚举或ID映射；
- 参数单位转换；
- 错误语义转换；
- 同步接口转异步提交；
- 隐藏平台单例；
- 从一个大接口裁剪出上层所需子集。

无价值的适配器只是逐参数转发：

```c
int layer_b_read(uint32_t a, uint8_t *d, uint32_t n)
{
    return layer_c_read(a, d, n);
}
```

如果它没有建立依赖边界或转换语义，只会增加调用深度和阅读成本。

## 9. 错误转换

不同层观察的错误粒度不同。设备层可能返回“写使能失败”或“DMA超时”，存储使用者只需要知道“存储操作失败”。

```c
switch (driver_result)
{
case DRIVER_OK:
    return STORAGE_OK;
case DRIVER_BUSY:
    return STORAGE_BUSY;
case DRIVER_BAD_ARGUMENT:
    return STORAGE_BAD_ARGUMENT;
default:
    return STORAGE_IO_ERROR;
}
```

错误转换不是丢弃诊断。平台仍可保存详细驱动错误，但公共接口只传播上层能够处理的语义。

## 10. 测试替身

函数表让核心逻辑可以挂载内存模拟器：

```c
static uint8_t fake_flash[4096];

static int fake_read(uint32_t offset,
                     uint8_t *data,
                     uint32_t length)
{
    memcpy(data, &fake_flash[offset], length);
    return 0;
}
```

测试可以控制busy持续时间、指定第几次操作失败、记录每次地址与长度。这样验证的是核心状态机和边界，而不是重新实现一份“专用测试版核心”。

## 11. 优点与代价

优点：

- 降低平台头文件向公共逻辑扩散。
- 便于替换驱动和构造测试替身。
- 显式表达模块真正需要的能力。
- 初始化时可以完整检查依赖。

代价：

- 间接调用降低静态调用图直观性。
- `void *`上下文缺少强类型保护。
- 过细函数表会制造大量薄适配层。
- 生命周期和并发仍需另外约定。

## 12. 思考题

1. 一个模块只调用一个固定函数，是否值得建立ops表？
2. 上层需要初始化下层，还是只需要下层已经可用？
3. 如果两个模块共享同一Flash状态机，谁有权调用stop？
4. 把全局单例隐藏在适配器中，对测试和多实例有什么影响？
5. 一个适配器既不转换类型也不转换语义，是否可以删除？
