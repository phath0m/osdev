/*
 * extract_tar.c 
 *
 * Extracts initial filesystem to ramdisk
 *
 * Copyright (C) 2020
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/vnode.h>

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
extract_file(struct vnode *root, const char *name, void *data, size_t size, mode_t mode)
{
    struct file *fp;

    if (vfs_creat(current_proc, &fp, name, mode) != 0) {
        panic("could not extract %s", name);
        return;
    }

    FOP_WRITE(fp, data, size);
    file_close(fp);
}

void
tar_extract_archive(struct vnode *root, struct vnode *cwd, const void *archive)
{
    int i;
    mode_t mode;
    size_t size;
    void *data;
    char name[100];

    const struct tar_header *header;

    printf("kernel: extracting files to ramdisk...\n\r");

    for (i = 0; ;i++) {
        header = archive;

        if (!*header->name) {
            break;
        }

        strcpy(name, header->name + 1);

        discard_trailing_slash(name);

        //node->header = header;
        data = (void*)(archive + 512);
        //node->gid = atoi(header->gid, 8);
        //node->mode = atoi(header->mode, 8);
        //node->uid = atoi(header->uid, 8);
        //node->mtime = atoi(header->mtime, 8);
        mode = atoi(header->mode, 8);
        size = atoi(header->size, 8);


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
                    vfs_mkdir(current_proc, name, mode);
                    break;
            }
        }
    }
}