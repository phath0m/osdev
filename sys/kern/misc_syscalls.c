#include <time.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/types.h>

// remove
#include <stdio.h>

static int
sys_time(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(time_t *, tloc, 0, argv);

    TRACE_SYSCALL("time", "0x%p", tloc);

    return time(tloc);
}

__attribute__((constructor))
void
_init_misc_syscalls()
{
    register_syscall(SYS_TIME, 1, sys_time);
}
