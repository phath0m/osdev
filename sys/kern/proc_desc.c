/*
 * proc_desc.c - manages process file descriptors
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
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/vnode.h>

int
procdesc_dup(int oldfd)
{
    int newfd;
    
    newfd = procdesc_getfd();

    return procdesc_dup2(oldfd, newfd);
}

int
procdesc_dup2(int oldfd, int newfd)
{
    struct file *existing_fp;
    struct file *fp;

    if (oldfd >= 4096 || newfd >= 4096) {
        return -1;
    }

    if (oldfd < 0 || newfd < 0) {
        return -1;
    }

    existing_fp = current_proc->files[newfd];

    if (existing_fp) {
        file_close(existing_fp);   
    }

    fp = current_proc->files[oldfd];
   
    if (!fp) {
        return -1;
    }

    current_proc->files[newfd] = fp;

    INC_FILE_REF(fp);
    
    return newfd;
}

struct file *
procdesc_getfile(int fildes)
{   
    if (fildes >= 4096) {
        return NULL;
    }
    return current_proc->files[fildes];
}

int
procdesc_getfd()
{   
    int i;

    for (i = 0; i < 4096; i++) { 
        if (!current_proc->files[i]) {
            return i;
        }
    }
    
    return -(EMFILE);
}

int
procdesc_newfd(struct file *file)
{
    int i;

    for (i = 0; i < 4096; i++) {
        if (!current_proc->files[i]) {
            current_proc->files[i] = file;
            return i;
        }
    }

    return -(EMFILE);
}

static int
get_fcntl_flags(struct file *fp)
{
    int ret;

    ret = 0;

    if ((fp->flags & O_CLOEXEC)) {
        ret |= FD_CLOEXEC; 
    }

    return ret;
}

static int
set_fcntl_flags(struct file *fp, int flags)
{
    if (flags & FD_CLOEXEC) {
        fp->flags |= O_CLOEXEC;
    }

    return 0;
}

int
procdesc_fcntl(int fd, int cmd, void *arg)
{
    struct file *fp;

    if (fd >= 4096 || fd >= 4096) {
        return -1;
    }

    fp = current_proc->files[fd];

    if (!fp) {
        return -1;
    }

    switch (cmd) {
        case F_SETFD:
            return set_fcntl_flags(fp, *((int*)arg));
        case F_GETFD:

            if (arg) {
                *((int*)arg) = get_fcntl_flags(fp);
            }

            return 0;
    }

    return -1;
}

