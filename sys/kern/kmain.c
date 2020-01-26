#include <ds/dict.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/vfs.h>

// remove
#include <sys/i686/vm.h>
#include <ds/membuf.h>

typedef void (*test_t)();

int
kmain()
{
    struct vfs_node *root;

    if (fops_openfs(NULL, &root, "tmpfs", 0) == 0) {
        printf("kernel: mounted initramfs to /\n");

        extern void *start_initramfs;
        extern void tar_extract_archive(struct vfs_node *root, struct vfs_node *cwd, const void *archive);

        root->mode = 755;
        current_proc->cwd = root;
        current_proc->root = root;
        
        tar_extract_archive(root, NULL, start_initramfs);

        if (vfs_mount(root, NULL, "devfs", "/dev", 0) == 0) {
            printf("kernel: mounted devfs to /dev\n");
        }

        /*
        if (vfs_mount(root, NULL, "tmpfs", "/tmp", 0) == 0) {
            printf("kernel: mounted tmpfs to /tmp\n");
        }*/

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
        "TERM=xterm",
        NULL,
    };

    printf("kernel: invoke /sbin/doit\n");

    current_proc->umask = 0744;

    proc_execve(argv[0], argv, envp);

    /*
     * There isn't much we can do right now :P
     */

    return 0;
}
