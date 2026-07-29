# Bootloader升级运行方法

本文说明现有工程中Bootloader与IAP两个固件角色如何运行升级。新平台需要加入哪些源码、如何实现Flash、分区、Adapter、链接和跳转，统一由[Bootloader平台移植方法](../porting/bootloader_platform_porting.md)说明。

## 1. 两个固件角色

同一产品保留两个构建目标：

| 目标 | 包含内容 | 外部升级命令 |
| --- | --- | --- |
| Bootloader/ISP | Bootloader Core、元数据、完整升级协议、FAL适配器 | `0x08`～`0x0B` |
| IAP | 正常应用、独立IAP升级触发服务 | 仅`0x08` |

IAP不编译完整Bootloader协议和安装状态机。Bootloader也不需要编译IAP内的准备回调服务。

## 2. 运行前提

Bootloader目标开始接收升级前应确认：

- FAL、Bootloader Core和通信Link均由周期任务持续推进；
- 目标模块ID、升级模式、镜像首部长度和静态缓冲区配置有效；
- IAP、暂存、元数据和布局区域容量满足当前镜像；
- Bootloader自身区域没有映射到在线升级接口；
- IAP目标只注册升级触发命令，并保留复位后不清零的升级请求区域。

这些能力的源码清单、初始化顺序和平台实现边界见平台移植文档，本文件从运行时升级请求开始说明。

## 3. 实现准备回调

应用覆盖弱定义函数：

```c
iap_update_prepare_result_t
iap_update_prepare(const iap_update_info_t *p_info)
{
    if (p_info == NULL)
    {
        return IAP_UPDATE_PREPARE_FAILED_E;
    }

    power_stage_disable();

    if (parameter_save_failed() != 0u)
    {
        return IAP_UPDATE_PREPARE_FAILED_E;
    }
    if (parameter_save_busy() != 0u)
    {
        return IAP_UPDATE_PREPARE_IN_PROGRESS_E;
    }

    return IAP_UPDATE_PREPARE_READY_E;
}
```

该函数由周期process重复调用：

- 首次调用可以发起封波、日志落盘或参数保存；
- 未完成返回`IN_PROGRESS`；
- 确认失败返回`FAILED`，不会复位；
- 所有准备动作完成后返回`READY`。

不要在回调中阻塞等待Flash或发送完成。

## 4. 首次升级

空IAP时设备应保持Bootloader通信可用：

1. FRAME发送`0x08`升级信息。
2. 轮询`0x09`直到Bootloader完成目标区域擦除。
3. 按offset顺序发送`0x0A`数据包。
4. 每包等待同命令ACK。
5. 发送`0x0B`及整包CRC。
6. Bootloader完成暂存安装或直接目标校验。
7. 元数据提交为有效后跳转IAP。

主机超时重发时只能重发上一offset。不能跳过尚未确认的数据包。

## 5. IAP内二次升级

1. FRAME向IAP发送`0x08`。
2. IAP直接返回接收ACK。
3. IAP周期执行用户准备回调。
4. 准备完成后写入保留SRAM请求并复位。
5. Bootloader读取请求并驻留。
6. FRAME重新建立Bootloader升级会话，继续`0x09`～`0x0B`。

IAP的`0x08` ACK只表示升级请求已接受，不表示已经复位或固件已经写入。

## 6. 运行异常处理

- `0x08`被拒绝：检查模块ID、文件大小、升级模式和区域容量。
- `0x09`长期未就绪：检查FAL是否周期推进及Flash状态函数是否退出busy。
- `0x0A`失败：检查offset、数据长度、包CRC和上一个写请求是否完成。
- 复位后仍进入IAP：检查保留SRAM段、magic/checksum和Bootloader启动原因读取。
- 暂存下载完成但不跳转：检查复制读回、完整CRC、footer及平台镜像首部。
- 失败后设备驻留Bootloader属于安全行为，应重新发送完整镜像，不要强制跳转。

## 7. 关联导航

### 源代码

- [Bootloader Core接口](../../../code/app/bootloader/core/bootloader_core.h)
- [升级元数据接口](../../../code/app/bootloader/core/bootloader_metadata.h)
- [FRAME升级协议接口](../../../code/app/bootloader/protocol/bootloader_protocol.h)
- [IAP升级触发服务](../../../code/app/bootloader/iap/iap_update_service.c)
- [HC32F334适配参考](../../../platform/hc32f334/bootloader/bootloader_fal_adapter.c)
- [Zynq-7020适配参考](../../../platform/zynq7020/ps/bootloader/bootloader_fal_adapter.c)

### 设计文档

- [Bootloader升级与防变砖设计](../../design/bootloader/bootloader_design.md)
- [FAL分区与异步Flash管理设计](../../design/storage/fal_design.md)
- [公共软件组件模型设计](../../design/framework/component_model.md)

### 相关应用

- [Bootloader平台移植方法](../porting/bootloader_platform_porting.md)
