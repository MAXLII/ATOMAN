# 通用 Section 链表与调试协议

## 数据结构

公共链表类型和操作接口定义在各运行模式的 `section.h`：

```c
typedef struct section_item
{
    void *p_obj;
    struct section_item *p_next;
} section_item_t;
```

`p_obj` 指向业务对象，`p_next` 只属于注册链表。裸机运行模式使用 `section_item_t *p_xxx_first` 保存首节点，扫描链接段时直接构造链表，不使用哨兵节点。任务等业务对象不再保存注册链指针。

## 注册展开

任务注册：

```c
REG_TASK_MS(10, control_task)
```

等价于以下结构：

```c
reg_task_t reg_task_control_task = {
    .t_period = 100u,
    .time_last = 0u,
    .p_func = control_task,
    .p_name = "control_task",
    .is_ready = 0u,
};

section_item_t section_item_reg_task_control_task = {
    .p_obj = (void *)&reg_task_control_task,
    .p_next = NULL,
};

SECTION_REG_ATTR_PREFIX const reg_section_t reg_section_reg_task_control_task
    SECTION_REG_ATTR_SUFFIX = {
    .section_type = (uint32_t)SECTION_TASK,
    .p_str = (void *)&section_item_reg_task_control_task,
};
```

链接段中的 `reg_section_t.p_str` 统一指向 `section_item_t`。初始化扫描按 `section_type` 选择目标链表，业务处理通过 `section_item_t.p_obj` 取得真实对象。

## 调试链表注册

`section_list_registration_t` 和 `REG_DBG_LIST` 定义在各运行模式的 `section.h` 中。链表表头由所属模块定义，并在表头声明处直接注册：

```c
section_item_t *p_task_first;
REG_DBG_LIST(task, p_task_first)
```

注册项只包含链表名称和首节点指针变量的地址。`REG_DBG_LIST` 将注册项包装为统一的 `section_item_t`，再以 `SECTION_DBG_LIST` 类型写入链接段。链表构造后首节点发生变化时，协议读取该指针变量即可获得当前链表。`code/dbg/section_list_service.c` 只负责扫描该类型的链接段记录并处理目录、节点查询协议，不声明具体链表表头，也不维护集中式链表清单。

当前由各链表所属模块分别注册 `init`、`task`、`interrupt`、`link`、`comm_command`、`comm_route`、`perf`、`scope`、`shell` 和 `sfra`。`list_id` 等于有效注册项在链接段中的顺序加 1。

所有链表节点均使用统一的 `section_item_t`。节点查询直接上传 `section_item_t.p_obj` 指向的业务对象地址，不按业务类型提取内部函数指针。

## 协议

指令集固定为 `0x01`，响应使用相同指令字并设置 `is_ack = 1`。多字节字段均为小端。

### `0x38` 链表目录

请求：

| 字段 | 长度 |
| --- | ---: |
| directory_index | 2 |

成功响应：

| 字段 | 长度 |
| --- | ---: |
| protocol_version | 1 |
| status | 1 |
| directory_index | 2 |
| list_count | 2 |
| list_id | 2 |
| node_count | 4 |
| name_len | 1 |
| name | name_len |

失败响应保留前 6 字节，用 `status` 返回原因。

### `0x39` 链表节点

请求：

| 字段 | 长度 |
| --- | ---: |
| list_id | 2 |
| node_index | 4 |

成功响应：

| 字段 | 长度 |
| --- | ---: |
| protocol_version | 1 |
| status | 1 |
| list_id | 2 |
| node_index | 4 |
| node_count | 4 |
| address | 4 |

失败响应不包含 `address`。`address` 是节点 `p_obj` 指向的注册对象地址，上位机使用该地址匹配 MAP 符号名称。
