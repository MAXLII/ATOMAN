/**
 * @file comm.c
 * @brief 通信协议（帧解析/CRC/路由/发送）
 */

#include "comm.h"
#include "section.h"
#include <string.h>
#include <stdlib.h>

section_com_t *p_com_first;
comm_route_t *p_comm_route_first;
extern section_link_t *p_link_first;

/* ------------------------ Comm / Route 插入 ------------------------ */
static void comm_insert(section_com_t *com)
{
    if (!com)
        return;

    com->p_next = NULL;
    if (!p_com_first)
        p_com_first = com;
    else
    {
        section_com_t *curr = p_com_first;
        while (curr->p_next)
            curr = (section_com_t *)curr->p_next;
        curr->p_next = com;
    }
}

static void comm_route_insert(comm_route_t *com)
{
    if (!com)
        return;

    com->p_next = NULL;
    if (!p_comm_route_first)
        p_comm_route_first = com;
    else
    {
        comm_route_t *curr = p_comm_route_first;
        while (curr->p_next)
            curr = curr->p_next;
        curr->p_next = com;
    }
}

void comm_init(void)
{
    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_COMM:
            comm_insert((section_com_t *)p->p_str);
            break;
        case SECTION_COMM_ROUTE:
            comm_route_insert((comm_route_t *)p->p_str);
            break;
        default:
            break;
        }
    }
}

REG_INIT(0, comm_init)

// ------------------------ CRC16（从原 section.c 迁入） ------------------------
static uint16_t crc16_table[256];

static void crc16_init_table(void)
{
    for (int i = 0; i < 256; i++)
    {
        uint16_t crc = 0;
        uint16_t c = i << 8;

        for (int j = 0; j < 8; j++)
        {
            if ((crc ^ c) & 0x8000)
            {
                crc = (crc << 1) ^ CRC16_CCITT_POLY;
            }
            else
            {
                crc = crc << 1;
            }
            c = c << 1;
        }

        crc16_table[i] = crc;
    }
}

// 保留你的 init 注册
REG_INIT(0, crc16_init_table)

uint16_t crc16_init(void) { return CRC16_CCITT_INIT; }
uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    uint8_t table_index = (crc >> 8) ^ data;
    return (crc << 8) ^ crc16_table[table_index];
}
uint16_t crc16_final(uint16_t crc) { return crc; }

uint16_t section_crc16(uint8_t *p_data, uint32_t len)
{
    uint16_t crc = crc16_init();
    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc16_update(crc, *(p_data + i));
    }
    return crc16_final(crc);
}

uint16_t section_crc16_with_crc(uint8_t *p_data, uint32_t len, uint16_t crc_in)
{
    uint16_t crc = crc_in;
    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc16_update(crc, *(p_data + i));
    }
    return crc;
}

// ------------------------ COMM 命令查找（从原 section.c 迁入） ------------------------
void (*find_comm_func(uint8_t cmd_set, uint8_t cmd_word))(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_com_t *p = p_com_first;
    section_com_t *p_last = NULL;
    while (p)
    {
        if ((p->cmd_set == cmd_set) &&
            (p->cmd_word == cmd_word))
        {
            if (p_last)
            {
                p_last->p_next = p->p_next; // 断开前一个节点的链接
                p->p_next = p_com_first;    // 将当前节点移到链表头部
                p_com_first = p;            // 更新链表头指针
            }
            return p->func;
        }
        p_last = p;
        p = (section_com_t *)p->p_next;
    }
    return NULL;
}

// ------------------------ 路由（从原 section.c 迁入） ------------------------
void comm_route_run(comm_ctx_t *p_comm_ctx)
{
    comm_route_t *p_comm_route = p_comm_route_first;

    while (p_comm_route)
    {
        if ((p_comm_ctx->link_id == p_comm_route->src_link_id) &&
            (p_comm_ctx->pack.dst == p_comm_route->dst_addr))
        {
            section_link_t *p_link = p_link_first;
            while (p_link)
            {
                if (p_comm_route->dst_link_id == p_link->link_id)
                {
                    comm_send_data(&p_comm_ctx->pack, p_link->my_printf);
                }
                p_link = p_link->p_next;
            }
        }
        p_comm_route = p_comm_route->p_next;
    }
}

// ------------------------ 协议解析状态机（从原 section.c 迁入） ------------------------
void comm_run(uint8_t data, DEC_MY_PRINTF, void *p)
{
    comm_ctx_t *p_comm_ctx = (comm_ctx_t *)p;
    switch (p_comm_ctx->status)
    {
    case SECTION_PACKFORM_STA_SOP: // 等待开始字符
        if (data == 0xE8)          // 假设'C'为开始字符
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_VER;                  // 切换到接收状态
            p_comm_ctx->crc = crc16_init();                                 // 重置CRC
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);          // 累加CRC
            p_comm_ctx->pack.sop = data;                                    // 记录开始字符
            p_comm_ctx->pack.p_data = (uint8_t *)p_comm_ctx->p_data_buffer; // 指向数据缓
            p_comm_ctx->is_route = 0;                                       // 清除路由标志
        }
        else
        {
            return; // 非开始字符
        }
        break;
    case SECTION_PACKFORM_STA_VER:
        p_comm_ctx->pack.version = data;
        p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
        if (p_comm_ctx->pack.version == 0x01)
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SRC;
            p_comm_ctx->src_flag = 0;
        }
        else
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 未找到对应的处理函数，重置状态
            return;                                        // 未找到对应的处理函数，直接返回
        }
        break;
    case SECTION_PACKFORM_STA_SRC:
        if (p_comm_ctx->src_flag == 0)
        {
            p_comm_ctx->pack.src = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            p_comm_ctx->src_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.d_src = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            p_comm_ctx->dst_flag = 0;
            p_comm_ctx->status = SECTION_PACKFORM_STA_DST;
        }
        break;
    case SECTION_PACKFORM_STA_DST:
        if (p_comm_ctx->dst_flag == 0)
        {
            p_comm_ctx->pack.dst = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            if ((p_comm_ctx->pack.dst == 0x00) ||
                (p_comm_ctx->pack.dst == p_comm_ctx->src))
            {
                p_comm_ctx->dst_flag = 1;
            }
            else
            {
                p_comm_ctx->is_route = 1;
                p_comm_ctx->dst_flag = 1;
            }
        }
        else
        {
            p_comm_ctx->pack.d_dst = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            if ((p_comm_ctx->pack.d_dst == 0x00) ||
                (p_comm_ctx->pack.d_dst == p_comm_ctx->d_src))
            {
                p_comm_ctx->cmd_flag = 0;
                p_comm_ctx->status = SECTION_PACKFORM_STA_CMD;
            }
            else if (p_comm_ctx->is_route == 1)
            {
                p_comm_ctx->cmd_flag = 0;
                p_comm_ctx->status = SECTION_PACKFORM_STA_CMD;
            }
            else
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_SOP;
                return;
            }
        }
        break;
    case SECTION_PACKFORM_STA_CMD: // 接收数据状态
        if (p_comm_ctx->cmd_flag == 0)
        {
            p_comm_ctx->pack.cmd_set = data;                       // 记录命令字
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            p_comm_ctx->cmd_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.cmd_word = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            if (p_comm_ctx->is_route == 0)
            {
                p_comm_ctx->func = find_comm_func(p_comm_ctx->pack.cmd_set,
                                                  p_comm_ctx->pack.cmd_word);
                if (p_comm_ctx->func == NULL)
                {
                    p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 未找到对应的处理函数，重置状态
                    return;                                        // 未找到对应的处理函数，直接返回
                }
                else
                {
                    p_comm_ctx->status = SECTION_PACKFORM_STA_ACK; // 切换到长度接收状态
                }
            }
            else
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_ACK; // 切换到长度接收状态
            }
        }
        break;
    case SECTION_PACKFORM_STA_ACK:
        p_comm_ctx->pack.is_ack = data;
        p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
        p_comm_ctx->pack.len = 0;                      // 重置长度
        p_comm_ctx->len_flag = 0;                      // 重置长度标志
        p_comm_ctx->status = SECTION_PACKFORM_STA_LEN; // 切换到长度接收状态
        break;
    case SECTION_PACKFORM_STA_LEN: // 接收长度状态
        if (p_comm_ctx->len_flag == 0)
        {
            p_comm_ctx->pack.len += data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            p_comm_ctx->len_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.len += (data << 8);                   // 累加长度
            p_comm_ctx->len = p_comm_ctx->pack.len;                // 设置数据长度
            p_comm_ctx->len_flag = 0;                              // 重置长度标志
            p_comm_ctx->status = SECTION_PACKFORM_STA_DATA;        // 切换到数据接收状态
            p_comm_ctx->index = 0;                                 // 重置索引
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            p_comm_ctx->pack.p_data = p_comm_ctx->p_data_buffer;   // 指向数据缓冲区
            // 检查长度是否超出缓冲区
            if (p_comm_ctx->len > p_comm_ctx->buffer_size)
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
                return;
            }
        }
        break;
    case SECTION_PACKFORM_STA_DATA: // 接收数据状态
        if (p_comm_ctx->len != 0)
        {
            p_comm_ctx->len--;
            uint8_t *p_data = p_comm_ctx->p_data_buffer + p_comm_ctx->index;
            *p_data = data;                                        // 存储数据
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // CRC
            p_comm_ctx->index++;
        }
        else
        {
            p_comm_ctx->pack.crc = 0;                      // 接收到数据长度为0，准备接收CRC
            p_comm_ctx->pack.crc += data;                  // 接收CRC
            p_comm_ctx->status = SECTION_PACKFORM_STA_CRC; // 切换到CRC接收状态
        }
        break;
    case SECTION_PACKFORM_STA_CRC:           // 接收CRC状态
        p_comm_ctx->pack.crc += (data << 8); // 累加CRC
        p_comm_ctx->crc = crc16_final(p_comm_ctx->crc);
        if (p_comm_ctx->crc == p_comm_ctx->pack.crc) // 匹配
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_EOP; // 切换到结束状态
            p_comm_ctx->eop_flag = 0;
            p_comm_ctx->pack.eop = 0; // 记录结束字符
        }
        else
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 不匹配，重置状态为等待开始字符
            return;
        }
        break;
    case SECTION_PACKFORM_STA_EOP:
        if (p_comm_ctx->eop_flag == 0)
        {
            p_comm_ctx->pack.eop += data; // 记录结束字符
            p_comm_ctx->eop_flag = 1;     // 设置结束标志
        }
        else if (p_comm_ctx->is_route == 1)
        {
            comm_route_run(p_comm_ctx);
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
        }
        else
        {
            p_comm_ctx->eop_flag = 0;            // 设置结束标志
            p_comm_ctx->pack.eop += (data << 8); // 记录结束字符
            if (p_comm_ctx->pack.eop == 0x0A0D)
            {
                // 调用处理函数
                if (p_comm_ctx->func)
                {
                    p_comm_ctx->func(&p_comm_ctx->pack, my_printf);
                }
            }
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
        }
        break;
    case SECTION_PACKFORM_STA_ROUTE:
        break;
    }
}

// ------------------------ 发送（从原 section.c 迁入） ------------------------
static uint8_t tx_buffer[512] = {0};
static int tx_buffer_index = 0;

static void write_tx_buffer(uint8_t data)
{
    if (tx_buffer_index >= (int)sizeof(tx_buffer))
        return;
    tx_buffer[tx_buffer_index++] = data;
}

void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (!p_pack)
        return;

    tx_buffer_index = 0;

    p_pack->version = 0x01;
    p_pack->crc = crc16_init();

    // 发送开始字符
    write_tx_buffer(0xE8);
    p_pack->crc = crc16_update(p_pack->crc, 0xE8);

    // 发送版本
    write_tx_buffer(p_pack->version);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->version);

    // 发送源地址
    write_tx_buffer(p_pack->src);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->src);

    // 发送动态源地址
    write_tx_buffer(p_pack->d_src);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_src);

    // 发送目的地址
    write_tx_buffer(p_pack->dst);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->dst);

    // 发送动态目的地址
    write_tx_buffer(p_pack->d_dst);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_dst);

    // 发送命令集
    write_tx_buffer(p_pack->cmd_set);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_set);

    // 发送命令字
    write_tx_buffer(p_pack->cmd_word);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_word);

    // 发送是响应
    write_tx_buffer(p_pack->is_ack);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->is_ack);

    // 发送长度
    write_tx_buffer((uint8_t)(p_pack->len & 0xFF));
    write_tx_buffer((uint8_t)((p_pack->len >> 8) & 0xFF));
    p_pack->crc = crc16_update(p_pack->crc, p_pack->len & 0xFF);
    p_pack->crc = crc16_update(p_pack->crc, (uint8_t)((p_pack->len >> 8) & 0xFF));

    // 发送数据
    if (p_pack->p_data)
    {
        for (uint32_t i = 0; i < p_pack->len; i++)
        {
            write_tx_buffer(p_pack->p_data[i]);
            p_pack->crc = crc16_update(p_pack->crc, p_pack->p_data[i]);
        }
    }

    // 发送CRC
    uint16_t crc = crc16_final(p_pack->crc);

    write_tx_buffer((uint8_t)(crc & 0xFF));
    write_tx_buffer((uint8_t)((crc >> 8) & 0xFF));

    write_tx_buffer(0x0D);
    write_tx_buffer(0x0A);

    if (my_printf)
        my_printf->tx_by_dma((char *)tx_buffer, tx_buffer_index);
}
