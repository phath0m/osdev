#include <ds/dict.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/vnode.h>

/* stage three of kernel initialization; exec /sbin/doit */
static int
exec_init(const char *args)
{
    const char *init_argv[] = {
        "/sbin/doit",
        args,
        NULL
    };

    
    const char *init_envp[] = {
        "CONSOLE=/dev/lfb",
        "TERM=xterm",
        NULL
    };

    printf("kernel: invoke /sbin/doit\n\r");

    current_proc->umask = 0744;

    proc_execve(init_argv[0], init_argv, init_envp);

    return 0;
}

/* stage two of the kernel initialization process; mount ramdisk */
int
init_thread(void *argp)
{
    /* this is sort of a hack because it assumes we're using LFB for output*/
    /* TODO: something that makes less assumptions */
    extern struct cdev lfb_device;

    set_kernel_output(&lfb_device);

    extern struct thread *sched_curr_thread;

    /* setup the first process */    
    struct proc *init = proc_new();
    struct session *init_session = session_new(init);
    struct pgrp *init_group = pgrp_new(init, init_session);
    
    init->thread = sched_curr_thread;
    init->group = init_group;
    
    sched_curr_thread->proc = init;
    current_proc = init;

    /* this is ugly and not how I want to do this (Initializing these filesystems
     * here). I'd prefer a more modular approach. We'll fix this some day
     */
    devfs_init();
    tmpfs_init();

    /* now mount the ramdisk */
    struct vnode *root;

    if (fs_open(NULL, &root, "tmpfs", 0) != 0) {
        panic("could not mount tmpfs!");
    }

    extern void *start_initramfs;
    extern void tar_extract_archive(struct vnode *root, struct vnode *cwd, const void *archive);

    root->mode = 0755;
    init->cwd = root;
    init->root = root;

    tar_extract_archive(root, NULL, start_initramfs);

    if (fs_mount(root, NULL, "devfs", "/dev", 0) != 0) {
        panic("could not mount devfs!");
    }

    exec_init((const char*)argp);

    return 0;
}

int
kmain(const char *args)
{
    /* initialize various pools for the various subsystems  */
    pool_init(&proc_pool, sizeof(struct proc), 0);
    pool_init(&thread_pool, sizeof(struct thread), 0);
    pool_init(&vn_pool, sizeof(struct vnode), 0);
    pool_init(&file_pool, sizeof(struct file), 0);

    /* initialize the socket subsystem */
    sock_init();

    /* initialize all system calls*/
    syscalls_init();

    thread_run((kthread_entry_t)init_thread, NULL, (void*)args);

    for (;;) {
        thread_yield();
    }

}
