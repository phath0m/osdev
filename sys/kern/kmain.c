#include <stdio.h>
#include <string.h>
#include <ds/dict.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/vfs.h>

// remove
#include <sys/i686/vm.h>

typedef void (*test_t)();

int
kmain()
{
    struct vfs_node *root;

    if (vfs_openfs(NULL, &root, "initramfs", MS_RDONLY) == 0) {
        printf("kernel: mounted initramfs to /\n");

        if (vfs_mount(root, NULL, "devfs", "/dev", 0) == 0) {
            printf("kernel: mounted devfs to /dev\n");
        }

        current_proc->cwd = root;
        current_proc->root = root;
    } else {
        panic("could not mount initramfs!\n");
    }
    
    extern struct vm_space *sched_curr_address_space;

    vm_map(sched_curr_address_space, (void*)0xFFFFF000, 0x1000, PROT_KERN | PROT_WRITE | PROT_READ);
    
    extern void set_tss_esp0(uintptr_t ptr);

    set_tss_esp0(0xFFFFFF00);

    const char *argv[] = {
        "/sbin/doit",
        NULL
    };


    const char *envp[] = {
        "CONSOLE=/dev/ttyS0",
        NULL,
    };

    extern int proc_chdir(const char *path);

    proc_execve("/sbin/doit", argv, envp);

    /*
     * There isn't much we can do right now :P
     */

    return 0;
}
