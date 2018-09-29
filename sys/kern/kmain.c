#include <rtl/dict.h>
#include <rtl/printf.h>
#include <rtl/string.h>
#include <sys/kernel.h>
#include <sys/vfs.h>

int
kmain()
{

    struct vfs_node *root;

    if (vfs_openfs(NULL, &root, "initramfs", MS_RDONLY) == 0) {

        struct file *file;

        if (vfs_mount(root, NULL, "devfs", "/dev", MS_RDONLY) == 0) {
            printf("mounted devfs!\n");
        }

        if (vfs_open(root, &file, "/dev", O_RDONLY) == 0) {
            struct dirent entry;

            while (vfs_readdirent(file, &entry)) {
                printf("Entry: %s\n", entry.name);
            }

            struct file *screen;

            if (vfs_open(root, &screen, "/dev/textscreen", O_WRONLY) == 0) {
                vfs_write(screen, "Hello, World", 12);
            }
            printf("Done!\n");
        }
    }


    /*
     * There isn't much we can do right now :P
     */
    asm volatile("cli");
    asm volatile("hlt");

    return 0;
}
