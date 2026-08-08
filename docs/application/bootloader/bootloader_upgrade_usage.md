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

## 6. Zynq-7020 网线升级

Zynq-7020 的 IAP 和 Bootloader 共用 PS GEM0 与 lwIP TCP 服务。设备固定地址为
`192.168.1.10/24`，FRAME 连接端口为 `9000`。上位机网卡地址需要配置在同一网段。

应用收到 `0x01/0x08` 后先通过当前 TCP 连接返回 ACK，保留 100 ms 发送窗口，然后主动
终止连接并跳转到 DDR 地址 `0x04000000` 的 Bootloader。升级启动原因保存在有效低地址
OCM 的 `0x0002FFF0`。Bootloader 重新初始化 GEM0 并监听相同地址和端口，FRAME 重连后
继续执行 `0x08`～`0x0B`。

Bootloader 使用 staged 模式。固件包先写入 QSPI staging 分区，完整 CRC16 与 34 字节
footer 校验通过后复制到 IAP 分区，再从 QSPI 加载到 `0x00100000` 并启动。在线升级不会
写入前 5 MiB 的受保护启动分区。

构建和生成启动镜像：

```powershell
cd D:\OneDrive\LWX\GD32\base\platform\zynq7020\ps\bootloader
.\compile.ps1
.\build_boot_image.ps1
```

输出文件为 `build/zynq7020_bootloader.elf`、`build/zynq7020_bootloader.bin` 和
`build/BOOT.bin`。`BOOT.bin` 包含 FSBL、PL bitstream 和 Bootloader。

JTAG 调试时可执行：

```powershell
& C:\Xilinx\SDK\2018.3\bin\xsct.bat .\download.tcl
```

该脚本把 Bootloader 下载到 DDR，并写入 IAP 升级启动原因，使 Bootloader 驻留等待网线
升级；它不修改 QSPI 启动分区。

## 7. 运行异常处理

- `0x08`被拒绝：检查模块ID、文件大小、升级模式和区域容量。
- `0x09`长期未就绪：检查FAL是否周期推进及Flash状态函数是否退出busy。
- `0x0A`失败：检查offset、数据长度、包CRC和上一个写请求是否完成。
- 复位后仍进入IAP：检查保留SRAM段、magic/checksum和Bootloader启动原因读取。
- 暂存下载完成但不跳转：检查复制读回、完整CRC、footer及平台镜像首部。
- 失败后设备驻留Bootloader属于安全行为，应重新发送完整镜像，不要强制跳转。

## 8. 关联导航

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
