#include <rtl/dict.h>
#include <rtl/printf.h>
#include <rtl/string.h>
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
    printf("Welcome to Elysium!\n");

    struct vfs_node *root;

    if (vfs_openfs(NULL, &root, "initramfs", MS_RDONLY) == 0) {
        printf("kernel: mounted initramfs to /\n");

        if (vfs_mount(root, NULL, "devfs", "/dev", 0) == 0) {
            printf("kernel: mounted devfs to /dev\n");
        }

        current_proc->root = root;
    }
    
    extern struct vm_space *sched_curr_address_space;

    vm_map(sched_curr_address_space, (void*)0xFFFFF000, 0x1000, PROT_KERN | PROT_WRITE | PROT_READ);
    
    extern void set_tss_esp0(uintptr_t ptr);

    set_tss_esp0(0xFFFFFF00);

    const char *argv[] = {
        "foo",
        "bar",
        NULL
    };

    printf("running init\n");
    proc_execve("/bin/test", argv, argv);

    /*
     * There isn't much we can do right now :P
     */

    return 0;
}
