#include <stdio.h>
#include <string.h>
#include <ds/dict.h>
#include <sys/kernel.h>
#include <sys/mount.h>
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

        if (vfs_mount(root, NULL, "tmpfs", "/tmp", 0) == 0) {
            printf("kernel: mounted tmpfs to /tmp\n");
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
        "CONSOLE=/dev/ttyS1",
        NULL,
    };

    extern int proc_chdir(const char *path);

    vfs_mkdir(root, "/tmp/test", 0777);
    vfs_mkdir(root, "/tmp/foo", 0700);
    vfs_mkdir(root, "/tmp/bar", 0710);
    vfs_rmdir(root, "/tmp/test");
    printf("kernel: invoke /sbin/doit\n");

    proc_execve(argv[0], argv, envp);

    /*
     * There isn't much we can do right now :P
     */

    return 0;
}
