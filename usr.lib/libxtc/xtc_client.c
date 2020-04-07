#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <libmemgfx/canvas.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "libxtc.h"
#include "xtcprotocol.h"

static int xtc_sock_fd = 0;

int
xtc_connect()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1) {
        return -1;
    }

    int flags;
    
    fcntl(fd, F_GETFD, &flags);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, &flags);

    struct sockaddr_un  addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, XTC_SOCK_PATH);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        return -1;
    }

    xtc_sock_fd = fd;

    return 0;    
}

void
xtc_disconnect()
{
    close(xtc_sock_fd);
}

int
xtc_next_event(xtc_win_t win, xtc_event_t *event)
{
    struct xtc_msg_hdr msg;
    msg.parameters[0] = win;
    msg.opcode = XTC_NEXTEVENT;
    msg.payload_size = 0;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));

    event->type = msg.parameters[0];
    event->parameters[0] = msg.parameters[1];
    event->parameters[1] = msg.parameters[2];
    
    return 0;
}

xtc_win_t
xtc_window_new(int width, int height, int flags)
{
    struct xtc_msg_hdr msg;
    
    msg.opcode = XTC_NEWWIN;
    msg.parameters[0] = width;
    msg.parameters[1] = height;
    msg.payload_size = 0;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));
 
    return msg.parameters[0];
}

int
xtc_clear(xtc_win_t win, int col)
{
    struct xtc_msg_hdr msg;

    msg.opcode = XTC_CLEAR;
    msg.parameters[0] = win;
    msg.parameters[1] = col;
    msg.payload_size = 0;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));

    return 0;
}

canvas_t *
xtc_open_canvas(xtc_win_t win, int flags)
{
    struct xtc_msg_hdr msg;

    msg.opcode = XTC_OPEN_CANVAS;
    msg.parameters[0] = win;

    msg.payload_size = 0;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));

    if (msg.payload_size == 0 || msg.error != 0) {
        return NULL;
    }
    
    char buf[512];

    read(xtc_sock_fd, buf, msg.payload_size);

    int fd = shm_open(buf, O_RDWR, 0);

    int width = msg.parameters[0];
    int height = msg.parameters[1];
    int size = msg.parameters[2];

    pixbuf_t *pixbuf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   
    close(fd);

    flags |= CANVAS_PARTIAL_RENDER;

    return canvas_from_mem(width, height, flags, pixbuf); 
}

int
xtc_put_string(xtc_win_t win, int x, int y, const char *str, int col)
{
    struct xtc_msg_hdr msg;

    msg.opcode = XTC_PUTSTRING;
    msg.parameters[0] = win;
    msg.parameters[1] = x;
    msg.parameters[2] = y;
    
    msg.payload_size = strlen(str);

    write(xtc_sock_fd, &msg, sizeof(msg));
    write(xtc_sock_fd, str, msg.payload_size);
    read(xtc_sock_fd, &msg, sizeof(msg));

    return 0;

}

int
xtc_set_window_title(xtc_win_t win, const char *title)
{
    struct xtc_msg_hdr msg;

    msg.opcode = XTC_SETWINTITLE;
    msg.parameters[0] = win;
    msg.payload_size = strlen(title);

    write(xtc_sock_fd, &msg, sizeof(msg));
    write(xtc_sock_fd, title, msg.payload_size);
    read(xtc_sock_fd, &msg, sizeof(msg));

    return 0;
}

int
xtc_redraw(xtc_win_t win)
{
    struct xtc_msg_hdr msg;

    msg.opcode = XTC_PAINT;
    msg.parameters[0] = win;
    msg.payload_size = 0;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));

    return 0;
}

int
xtc_resize(xtc_win_t win, color_t fillcolor, int width, int height)
{
    struct xtc_msg_hdr msg;
    msg.opcode = XTC_RESIZE;
    msg.parameters[0] = win;
    msg.parameters[1] = width;
    msg.parameters[2] = height;
    msg.parameters[3] = fillcolor;

    write(xtc_sock_fd, &msg, sizeof(msg));
    read(xtc_sock_fd, &msg, sizeof(msg));
    
    return 0;
}
