#include <ds/dict.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/vnode.h>

int
kmain(const char *args)
{
    /* this is sort of a hack because it assumes we're using LFB for output*/
    /* TODO: something that makes less assumptions */
    //extern struct device lfb_device;
    
    //kset_output(&lfb_device);

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
    }

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
