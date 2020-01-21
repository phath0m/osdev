#include <time.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/utsname.h>

// remove
#include <stdio.h>

static int
sys_time(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(time_t *, tloc, 0, argv);

    TRACE_SYSCALL("time", "0x%p", tloc);

    return time(tloc);
}

static int
sys_uname(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(struct utsname *, buf, 0, argv);

    TRACE_SYSCALL("uname", "0x%p", buf);

    strncpy(buf->sysname, ELYSIUM_SYSNAME, sizeof(buf->sysname));
    strncpy(buf->release, ELYSIUM_RELEASE, sizeof(buf->release));
    strncpy(buf->version, ELYSIUM_VERSION, sizeof(buf->version));
    strncpy(buf->machine, ELYSIUM_TARGET, sizeof(buf->machine));
    
    return 0;
}

__attribute__((constructor))
void
_init_misc_syscalls()
{
    register_syscall(SYS_TIME, 1, sys_time);
    register_syscall(SYS_UNAME, 1, sys_uname);
}
