// SPDX-License-Identifier: MIT
/**
 * @file    syscalls.c
 * @brief   Newlib system-call adaptation for the GD32E507 demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Provide the minimal system calls required by newlib-nano
 *          - Bound heap growth to the linker-defined heap region
 *          - Report character-device semantics without host file access
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation outside the linker-defined heap
 *          - System calls are not used from ISR paths
 *          - Hardware access is not required by this adaptation
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

int _close(int file_descriptor);
int _fstat(int file_descriptor, struct stat *p_status);
pid_t _getpid(void);
int _isatty(int file_descriptor);
int _kill(pid_t process_id, int signal_number);
off_t _lseek(int file_descriptor, off_t offset, int origin);
ssize_t _read(int file_descriptor, void *p_buffer, size_t length);
void *_sbrk(ptrdiff_t increment);
ssize_t _write(int file_descriptor, const void *p_buffer, size_t length);
void _exit(int status) __attribute__((noreturn));

int _close(int file_descriptor)
{
    (void)file_descriptor;
    errno = EBADF;
    return -1;
}

int _fstat(int file_descriptor, struct stat *p_status)
{
    (void)file_descriptor;

    if (p_status == NULL)
    {
        errno = EFAULT;
        return -1;
    }

    p_status->st_mode = S_IFCHR;
    return 0;
}

pid_t _getpid(void)
{
    return (pid_t)1;
}

int _isatty(int file_descriptor)
{
    (void)file_descriptor;
    return 1;
}

int _kill(pid_t process_id, int signal_number)
{
    (void)process_id;
    (void)signal_number;
    errno = EINVAL;
    return -1;
}

off_t _lseek(int file_descriptor, off_t offset, int origin)
{
    (void)file_descriptor;
    (void)offset;
    (void)origin;
    return (off_t)0;
}

ssize_t _read(int file_descriptor, void *p_buffer, size_t length)
{
    (void)file_descriptor;
    (void)p_buffer;
    (void)length;
    return (ssize_t)0;
}

void *_sbrk(ptrdiff_t increment)
{
    extern char _end[];
    extern char _heap_end[];
    static char *p_heap_current = _end;
    char *p_previous = p_heap_current;

    if (((increment > 0) && (increment > (_heap_end - p_heap_current))) ||
        ((increment < 0) && (increment < (_end - p_heap_current))))
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    p_heap_current += increment;
    return p_previous;
}

ssize_t _write(int file_descriptor, const void *p_buffer, size_t length)
{
    (void)file_descriptor;
    (void)p_buffer;
    return (ssize_t)length;
}

void _exit(int status)
{
    (void)status;

    for (;;)
    {
    }
}
