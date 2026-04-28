#include <errno.h>
#include <stddef.h>

#if !defined(__CC_ARM) && !defined(__ARMCC_VERSION) && !defined(__ARMCOMPILER_VERSION)
#include <sys/stat.h>
#include <sys/types.h>
#else
struct stat
{
    int st_mode;
};

#ifndef S_IFCHR
#define S_IFCHR 0x2000
#endif
#endif

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;

    if (st == NULL) {
        errno = EINVAL;
        return -1;
    }

    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _write(int file, char *ptr, int len)
{
    (void)file;

    if (ptr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return len;
}
