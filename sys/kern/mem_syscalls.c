#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/shm.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vm.h>

struct mmap_args {
    uintptr_t   addr;
    size_t      length;
    int         prot;
    int         flags;
    int         fd;
    off_t       offset;
};

static int
sys_mmap(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(struct mmap_args*, args, 0, argv);

    TRACE_SYSCALL("mmap", "0x%p, %d, 0x%p, 0x%p, %d, %d", args->addr, args->length, args->prot, args->flags, args->fd, args->offset);

    struct file *file = proc_getfile(args->fd);

    if (!file) {
        return -(EBADF);
    }

    return vops_mmap(file, args->addr, args->length, args->prot, args->offset);
}

static int
sys_munmap(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(void *, addr, 0, argv);
    DEFINE_SYSCALL_PARAM(size_t, length, 1, argv);

    TRACE_SYSCALL("munmap", "0x%p, %d", addr, length);

    extern struct vm_space *sched_curr_address_space;

    vm_unmap(sched_curr_address_space, addr, length);

    return 0;
}

static int
sys_shm_open(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, name, 0, argv);
    DEFINE_SYSCALL_PARAM(int, oflag, 1, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 2, argv);

    TRACE_SYSCALL("shm_open", "\"%s\", %d, %d", name, oflag, mode);

    struct file *fp;

    int res = shm_open(current_proc, &fp, name, oflag, mode);

    if (res != 0) {
        return res;
    }

    int fd = proc_newfildes(fp);

    return fd;
}

static int
sys_shm_unlink(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, name, 0, argv);
   
    TRACE_SYSCALL("shm_unlink", "\%s\"", name);

    return shm_unlink(current_proc, name);
}

__attribute__((constructor))
void
_init_mem_syscalls()
{
    register_syscall(SYS_MMAP, 1, sys_mmap);
    register_syscall(SYS_MUNMAP, 2, sys_munmap);
    register_syscall(SYS_SHM_OPEN, 3, sys_shm_open);
    register_syscall(SYS_SHM_UNLINK, 1, sys_shm_unlink);
}
