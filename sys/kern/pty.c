#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/vfs.h>

#define PTY_LINEBUF_SIZE    4096

static int ptm_close(struct vfs_node *node, struct file *fp);
static int ptm_destroy(struct vfs_node *node);
static int ptm_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int ptm_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);
static int pts_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
static int pts_isatty(struct device *dev);
static int pts_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos);
static int pts_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

struct file_ops ptm_ops = {
    .close      = ptm_close,
    .destroy    = ptm_destroy,
    .read       = ptm_read,
    .write      = ptm_write
};


struct pty {
    struct file *   input_pipe[2];
    struct file *   output_pipe[2];
    struct device * slave;
    struct termios  termios;
    int             line_buf_pos;
    char            line_buf[PTY_LINEBUF_SIZE];
};

static int pty_counter = 0;

static struct device *
mkpty_slave(struct pty *pty)
{
    struct device *pts_dev = calloc(1, sizeof(struct device) + 14);
    char *name = (char*)&pts_dev[1];

    sprintf(name, "pt%d", pty_counter++);

    pts_dev->name = name;

    pts_dev->mode = 0600;
    pts_dev->state = pty;
    pts_dev->ioctl = pts_ioctl;
    pts_dev->isatty = pts_isatty;
    pts_dev->read = pts_read;
    pts_dev->write = pts_write;

    device_register(pts_dev);

    return pts_dev;
}

struct file *
mkpty()
{
    struct pty *pty = calloc(1, sizeof(struct pty));

    pty->termios.c_lflag = ECHO | ICANON;
    pty->termios.c_oflag = OPOST;
    create_pipe(pty->input_pipe);
    create_pipe(pty->output_pipe);

    pty->slave = mkpty_slave(pty);

    struct vfs_node *node = vfs_node_new(NULL, &ptm_ops);

    node->state = pty;
    node->device = pty->slave;
    struct file *res = file_new(node);

    res->flags = O_RDWR;

    return res;
}


static void
pty_flush_buf(struct pty *pty)
{
    fops_write(pty->input_pipe[1], pty->line_buf, pty->line_buf_pos);

    pty->line_buf_pos = 0;
}

static void
pty_outprocess(struct pty *pty, const char *buf, size_t nbyte)
{
    const char *last_chunk = buf;
    int last_chunk_pos = 0;

    for (int i = 0; i < nbyte; i++) {
        if (buf[i] == '\n') {
            if (i != last_chunk_pos) fops_write(pty->output_pipe[1], last_chunk, i - last_chunk_pos);
            fops_write(pty->output_pipe[1], "\r\n", 2);
            last_chunk = &buf[i + 1];
            last_chunk_pos = i + 1;
        }
    }

    if(last_chunk_pos < nbyte) {
        fops_write(pty->output_pipe[1], last_chunk, nbyte - last_chunk_pos);
    }
}

static
int ptm_close(struct vfs_node *node, struct file *fp)
{
    return 0;
}

static int
ptm_destroy(struct vfs_node *node)
{
    return 0;
}

static
int ptm_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)node->state;

    return fops_read(pty->output_pipe[0], buf, nbyte);
}

static int
ptm_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)node->state;
    uint8_t *buf8 = (uint8_t*)buf;

    if (pty->termios.c_lflag & ECHO) {
        if (pty->termios.c_oflag & OPOST) {
            pty_outprocess(pty, buf, nbyte);
        } else {
            fops_write(pty->output_pipe[1], buf, nbyte);
        }
    }

    if (!(pty->termios.c_lflag & ICANON)) {
        return fops_write(pty->input_pipe[1], buf, nbyte);
    }

    for (int i = 0; i < nbyte; i++) {
        if (buf8[i] == '\b' && pty->line_buf_pos > 0) {
            pty->line_buf[pty->line_buf_pos - 1] = 0;
            pty->line_buf_pos--;
        } else if (buf8[i] == '\r' || pty->line_buf_pos > PTY_LINEBUF_SIZE) {
            pty->line_buf[pty->line_buf_pos++] = '\n';
            fops_write(pty->output_pipe[1], "\r\n", 2);
            pty_flush_buf(pty);
        } else {
            pty->line_buf[pty->line_buf_pos++] = buf8[i];
        }
    }
    
    return nbyte;
}

static int
pts_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
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
    case TIOCGWINSZ:
        ((struct winsize*)argp)->ws_row = 50;
        ((struct winsize*)argp)->ws_col = 100;
        break; 
    default:
        printf("got unknown IOCTL %d!\n", request);
        break;
    }

    return 0;
}

static int
pts_isatty(struct device *dev)
{
        return 1;
}

static int
pts_read(struct device *dev, char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    return fops_read(pty->input_pipe[0], buf, nbyte);
}

static int
pts_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct pty *pty = (struct pty*)dev->state;

    if (!(pty->termios.c_oflag & OPOST)) {
        return fops_write(pty->output_pipe[1], buf, nbyte);
    }

    pty_outprocess(pty, buf, nbyte);

    return nbyte;
}
