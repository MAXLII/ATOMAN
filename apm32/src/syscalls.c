/* @file    syscalls.c
 * @brief   Newlib系统调用实现文件.
 * @details
 *          This file is part of the PFC project.
 *
 *          Module responsibilities:
 *          - 实现Newlib所需的标准系统调用
 *          - 提供嵌入式环境的文件系统抽象
 *          - 实现进程管理相关函数
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - 所有文件操作返回失败或固定值（嵌入式环境无真实文件系统）
 *          - _exit实现为死循环（禁止程序退出）
 *
 * @author  Max.Li
 * @date    2024-03-27
 * @version 1.0.0
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

/* 关闭文件系统调用
 * ptr: reent结构体指针（未使用）
 * fd: 文件描述符（未使用）
 *
 * 返回：-1（总是失败，嵌入式环境不支持文件操作） */
int _close_r(struct _reent *ptr, int fd)
{
    (void)ptr;
    (void)fd;
    return -1;
}

/* 获取文件状态系统调用
 * ptr: reent结构体指针（未使用）
 * fd: 文件描述符（未使用）
 * pstat: 文件状态结构体指针
 *
 * 返回：0（成功，文件状态设为字符设备） */
int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    (void)ptr;
    (void)fd;
    pstat->st_mode = S_IFCHR;
    return 0;
}

/* 检查是否为终端设备系统调用
 * ptr: reent结构体指针（未使用）
 * fd: 文件描述符（未使用）
 *
 * 返回：1（总是返回真，表示是终端设备） */
int _isatty_r(struct _reent *ptr, int fd)
{
    (void)ptr;
    (void)fd;
    return 1;
}

/* 移动文件指针系统调用
 * ptr: reent结构体指针（未使用）
 * fd: 文件描述符（未使用）
 * pos: 偏移量（未使用）
 * whence: 起始位置（未使用）
 *
 * 返回：0（总是返回0，嵌入式环境无真实文件系统） */
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
    (void)ptr;
    (void)fd;
    (void)pos;
    (void)whence;
    return 0;
}

/* 读取文件系统调用
 * ptr: reent结构体指针（未使用）
 * fd: 文件描述符（未使用）
 * buf: 读取缓冲区（未使用）
 * cnt: 要读取的字节数（未使用）
 *
 * 返回：0（总是返回0，表示读取0字节） */
ssize_t _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    (void)ptr;
    (void)fd;
    (void)buf;
    (void)cnt;
    return 0;
}

ssize_t _write(int fd, const void *buf, size_t cnt)
{
    (void)fd;
    (void)buf;
    return (ssize_t)cnt;
}

/* 获取进程时间系统调用
 * ptr: reent结构体指针（未使用）
 * times: 时间结构体指针（未使用）
 *
 * 返回：-1（表示失败） */
clock_t _times_r(struct _reent *ptr, struct tms *times)
{
    (void)ptr;
    (void)times;
    return -1;
}

/* 获取文件状态系统调用（路径版本）
 * ptr: reent结构体指针（未使用）
 * file: 文件路径（未使用）
 * pstat: 文件状态结构体指针
 *
 * 返回：0（成功，文件状态设为字符设备） */
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
    (void)ptr;
    (void)file;
    pstat->st_mode = S_IFCHR;
    return 0;
}

/* 退出程序系统调用
 * status: 退出状态码（未使用）
 *
 * 功能说明：
 * - 嵌入式环境不允许程序真正退出
 * - 进入死循环等待看门狗复位
 * - 使用NOP指令防止编译器优化 */
void _exit(int status)
{
    (void)status;
    while (1)
    {
        __asm__("nop");
    }
}

/* 杀死进程系统调用
 * ptr: reent结构体指针（未使用）
 * pid: 进程ID（未使用）
 * sig: 信号编号（未使用）
 *
 * 返回：-1（总是失败，嵌入式环境不支持进程管理） */
int _kill_r(struct _reent *ptr, int pid, int sig)
{
    (void)ptr;
    (void)pid;
    (void)sig;
    return -1;
}

/* 获取进程ID系统调用
 * ptr: reent结构体指针（未使用）
 *
 * 返回：1（总是返回1，单进程环境） */
int _getpid_r(struct _reent *ptr)
{
    (void)ptr;
    return 1;
}
