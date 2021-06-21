/*
 * fifo.c - named pipe implementation
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
#include <sys/pipe.h>
#include <sys/systm.h>
#include <sys/vnode.h>

static int fifo_close(struct file *);
static int fifo_destroy(struct file *);
static int fifo_duplicate(struct file *);
static int fifo_read(struct file *, void *, size_t);
static int fifo_stat(struct file *, struct stat *);
static int fifo_write(struct file *, const void *, size_t);

struct fops fifo_ops = {
    .close      = fifo_close,
    .destroy    = fifo_destroy,
    .duplicate  = fifo_duplicate,
    .read       = fifo_read,
    .stat       = fifo_stat,
    .write      = fifo_write
};

struct fifo {
    struct file *       pipe[2];
    struct vnode *      host;
    int                 write_refs;
    int                 read_refs;
};

static struct file *
fifo_open_write(struct vnode *vn)
{
    struct fifo *reader;

    while (LIST_SIZE(&vn->un.fifo_readers) == 0) {
        thread_yield();
    }

    reader = list_peek_back(&vn->un.fifo_readers);

    reader->write_refs++;

    return file_new(&fifo_ops, reader);
}

static struct file *
fifo_open_read(struct vnode *vn)
{
    struct fifo *fifo;
    struct file *fp;

    fifo = calloc(1, sizeof(struct fifo));

    fifo->read_refs = 1;
    fifo->host = vn;

    create_pipe(fifo->pipe);

    fp = file_new(&fifo_ops, fifo);

    fp->state = fifo;

    list_append(&vn->un.fifo_readers, fifo);

    return fp;
}

struct file *
fifo_to_file(struct vnode *vn, mode_t mode)
{
    if ((mode & O_WRONLY)) {
        return fifo_open_write(vn);
    }

    return fifo_open_read(vn);
}

static int
fifo_close(struct file *fp)
{
    struct fifo *fifo;
    struct vnode *vn;

    fifo = fp->state;
    vn = fifo->host;

    if ((fp->flags & O_WRONLY)) {
        fifo->write_refs--; 
    } else {
        fifo->read_refs--;
    }

    if (fifo->write_refs == 0) {
        fop_close(fifo->pipe[1]);
        fifo->write_refs--;
    }

    if (fifo->read_refs == 0) {
        fop_close(fifo->pipe[0]);
        list_remove(&vn->un.fifo_readers, fifo);
        fifo->read_refs--;
    }

    return 0;
}

static int
fifo_destroy(struct file *fp)
{
    return 0;
}

static int
fifo_duplicate(struct file *fp)
{
    struct fifo *fifo;
    
    fifo = fp->state;

    if (fp->flags & O_WRONLY) {
        fifo->write_refs++;
    } else {
        fifo->read_refs++;
    }

    return 0;
}

static int
fifo_read(struct file *fp, void *buf, size_t nbyte)
{
    struct fifo *fifo;
    
    fifo = fp->state;

    return fop_read(fifo->pipe[0], buf, nbyte);
}

static int
fifo_stat(struct file *fp, struct stat *stat)
{
    struct fifo *fifo;
    struct vnode *vn;
    struct vops *ops;

    fifo = fp->state;
    vn = fifo->host;
    ops = vn->ops;

    if (ops->stat) {
        return ops->stat(vn, stat);
    }

    return -(ENOTSUP);
}

static int
fifo_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct fifo *fifo;
    
    fifo = (struct fifo*)fp->state;

    return fop_write(fifo->pipe[1], buf, nbyte);
}
