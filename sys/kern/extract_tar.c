#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/vfs.h>

#define TAR_REG     '0'
#define TAR_SYMLINK '2'
#define TAR_DIR     '5'

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
};

static inline void
discard_trailing_slash(char *path)
{
    while (*path) {
        if (*path == '/' && path[1] == 0) {
            *path = 0;
            return;
        }
        path++;
    }
}

static void
extract_file(struct vfs_node *root, const char *name, void *data, size_t size, mode_t mode)
{
    struct file *fp;

    if (fops_creat(root, NULL, &fp, name, mode) != 0) {
        printf("error: extracting %s\n", name);
        return;
    }

    fops_write(fp, data, size);

    fops_close(fp);
}

void
tar_extract_archive(struct vfs_node *root, struct vfs_node *cwd, const void *archive)
{
    for (int i = 0; ;i++) {
        struct tar_header *header = (struct tar_header*)archive;

        if (!*header->name) {
            break;
        }

        char name[100];

        strcpy(name, header->name + 1);

        discard_trailing_slash(name);

        //node->header = header;
        void *data = (void*)(archive + 512);
        //node->gid = atoi(header->gid, 8);
        //node->mode = atoi(header->mode, 8);
        //node->uid = atoi(header->uid, 8);
        //node->mtime = atoi(header->mtime, 8);
        mode_t mode = atoi(header->mode, 8);
        size_t size = atoi(header->size, 8);


        archive += ((size / 512) + 1) * 512;

        if (size % 512 != 0) {
            archive += 512;
        }

        if (i > 0) {
            switch (header->typeflag) {
                case TAR_REG:
                    extract_file(root, name, data, size, mode);
                    break;
                case TAR_DIR:
                    fops_mkdir(root, NULL, name, mode);
                    break;
            }
        }
    }
}
