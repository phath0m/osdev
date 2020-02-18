#include <ds/dict.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/vnode.h>

// remove
#include <sys/i686/vm.h>
#include <ds/membuf.h>

typedef void (*test_t)();

int
kmain()
{
    /* this is sort of a hack because it assumes we're using LFB for output*/
    /* TODO: something that makes less assumptions */
    extern struct device lfb_device;
    
    kset_output(&lfb_device);

    struct vnode *root;

    if (fs_open(NULL, &root, "tmpfs", 0) != 0) {
        panic("could not mount tmpfs!");
    }

    extern void *start_initramfs;
    extern void tar_extract_archive(struct vnode *root, struct vnode *cwd, const void *archive);

    root->mode = 0755;
    current_proc->cwd = root;
    current_proc->root = root;
        
    tar_extract_archive(root, NULL, start_initramfs);

    if (fs_mount(root, NULL, "devfs", "/dev", 0) != 0) {
        panic("could not mount devfs!");
        printf("kernel: mounted devfs to /dev\n");
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
        "TERM=xterm",
        NULL
    };

    printf("kernel: invoke /sbin/doit\n");

    current_proc->umask = 0744;

    proc_execve(argv[0], argv, envp);

    /*
     * There isn't much we can do right now :P
     */

    return 0;
}
