/*
 * pty.c - kernel pseudo-terminal implementation
 *
 * This file contains the pty implementation for the kernel and is responsible
 * for the creation of pseudo-terminal devices (/dev/ptX) 
 *
 */
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/pipe.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/vnode.h>

#define PTY_LINEBUF_SIZE    4096

static int ptm_close(struct file *fp);
static int ptm_getdev(struct file *fp, struct cdev **result);
static int ptm_read(struct file *fp, void *buf, size_t nbyte);
static int ptm_write(struct file *fp, const void *buf, size_t nbyte);
static int pts_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
static int pts_isatty(struct cdev *dev);
static int pts_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
static int pts_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

struct fops ptm_ops = {
    .close      = ptm_close,
    .getdev     = ptm_getdev,
    .read       = ptm_read,
    .write      = ptm_write
};

struct pty {
    struct file *   input_pipe[2];
    struct file *   output_pipe[2];
    struct cdev *   slave;
    struct termios  termios;
    int             line_buf_pos;
    char            line_buf[PTY_LINEBUF_SIZE];
    struct winsize  winsize;
    pid_t           foreground;
};

static int pty_counter = 0;

static struct cdev *
mkpty_slave(struct pty *pty)
{
    struct cdev *pts_dev = calloc(1, sizeof(struct cdev) + 14);
    char *name = (char*)&pts_dev[1];

    sprintf(name, "pt%d", pty_counter++);

    pts_dev->name = name;

    pts_dev->mode = 0600;
    pts_dev->majorno = DEV_MAJOR_PTS;
    pts_dev->minorno = pty_counter;
    pts_dev->state = pty;
    pts_dev->ioctl = pts_ioctl;
    pts_dev->isatty = pts_isatty;
    pts_dev->read = pts_read;
    pts_dev->write = pts_write;

    cdev_register(pts_dev);

    return pts_dev;
}

struct file *
mkpty()
{
    struct pty *pty = calloc(1, sizeof(struct pty));
    
    pty->winsize.ws_row = 50;
    pty->winsize.ws_col = 100;
    pty->termios.c_lflag = ECHO | ICANON;
    pty->termios.c_oflag = OPOST;

    create_pipe(pty->input_pipe);
    create_pipe(pty->output_pipe);

    pty->slave = mkpty_slave(pty);

    struct file *res = file_new(&ptm_ops, NULL);

    res->state = pty;

    res->flags = O_RDWR;

    return res;
}

static void
pty_flush_buf(struct pty *pty)
{
    fop_write(pty->input_pipe[1], pty->line_buf, pty->line_buf_pos);

    pty->line_buf_pos = 0;
}

static void
pty_outprocess(struct pty *pty, const char *buf, size_t nbyte)
{
    const char *last_chunk = buf;
    int last_chunk_pos = 0;

    for (int i = 0; i < nbyte; i++) {
        if (buf[i] == '\n') {
            if (i != last_chunk_pos) fop_write(pty->output_pipe[1], last_chunk, i - last_chunk_pos);
            fop_write(pty->output_pipe[1], "\r\n", 2);
            last_chunk = &buf[i + 1];
            last_chunk_pos = i + 1;
        }
    }

    if(last_chunk_pos < nbyte) {
        fop_write(pty->output_pipe[1], last_chunk, nbyte - last_chunk_pos);
    }
}

static int
ptm_close(struct file *fp)
{
    return 0;
}

static int
ptm_getdev(struct file *fp, struct cdev **result)
{
    struct pty *pty = (struct pty*)fp->state;

    *result = pty->slave;

    return 0;
}

/*
static int
ptm_destroy(struct vnode *node)
{
    return 0;
}
*/

static int
ptm_read(struct file *fp, void *buf, size_t nbyte)
{
    struct pty *pty = (struct pty*)fp->state;

    return fop_read(pty->output_pipe[0], buf, nbyte);
}

static int
ptm_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct pty *pty = (struct pty*)fp->state;
    uint8_t *buf8 = (uint8_t*)buf;

    if (pty->termios.c_lflag & ECHO) {
        if (pty->termios.c_oflag & OPOST) {
            pty_outprocess(pty, buf, nbyte);
        } else {
            fop_write(pty->output_pipe[1], buf, nbyte);
        }
    }

    if (!(pty->termios.c_lflag & ICANON)) {
        return fop_write(pty->input_pipe[1], buf, nbyte);
    }

    for (int i = 0; i < nbyte; i++) {
        if (buf8[i] == '\b' && pty->line_buf_pos > 0) {
            pty->line_buf[pty->line_buf_pos - 1] = 0;
            pty->line_buf_pos--;
        } else if (buf8[i] == '\r' || pty->line_buf_pos > PTY_LINEBUF_SIZE) {
            pty->line_buf[pty->line_buf_pos++] = '\n';
            fop_write(pty->output_pipe[1], "\r\n", 2);
            pty_flush_buf(pty);
        } else {
            pty->line_buf[pty->line_buf_pos++] = buf8[i];
        }
    }
    
    return nbyte;
}

static int
pts_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    struct pty *pty = (struct pty*)dev->state; 
    switch (request) {
    case TCGETS:
        memcpy((void*)argp, &pty->termios, sizeof(struct termios));
        break;
    case TCSETS:
        memcpy(&pty->termios, (void*)argp, sizeof(struct termios));
        break;
    case TCSETSW:
        memcpy(&pty->termios, (void*)argp, sizeof(struct termios));
        break;
    case TCSETSF:
        pty_flush_buf(pty);
        memcpy(&pty->termios, (void*)argp, sizeof(struct termios));
        break;
    case TIOCSWINSZ:
        memcpy(&pty->winsize, (void*)argp, sizeof(struct winsize));
        break;
    case TIOCGWINSZ:
        memcpy((void*)argp, &pty->winsize, sizeof(struct winsize));
        break; 
    case TIOCSCTTY: {
        struct session *session = PROC_GET_SESSION(current_proc);
    
        if (!session->ctty) {
            session->ctty = dev;
            return 0;
        }

        return -1;
    }
    case TIOCSPGRP: {
        pid_t pgid = *((pid_t*)argp);
        struct pgrp *pgrp = pgrp_find(pgid);

        if (pgrp) {
            pty->foreground = pgid;
            return 0;
        }

        return -(ESRCH);
    }
    case TIOCGPGRP: {
        *((pid_t*)argp) = pty->foreground;
        
        return 0;
    }
    default:
        printf("got unknown IOCTL %d!\n", request);
        break;
    }

    return 0;
}

static int
pts_isatty(struct cdev *dev)
{
        return 1;
}

static int
pts_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    return fop_read(pty->input_pipe[0], buf, nbyte);
}

static int
pts_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    if (!(pty->termios.c_oflag & OPOST)) {
        return fop_write(pty->output_pipe[1], buf, nbyte);
    }

    pty_outprocess(pty, buf, nbyte);

    return nbyte;
}
