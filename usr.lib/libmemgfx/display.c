#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "canvas.h"
#include "display.h"

display_t *
display_open()
{
    
    int fd = open("/dev/lfb", O_RDWR);

    if (fd == -1) {
        return NULL;
    }

    struct lfb_info fbinfo;

    ioctl(fd, FBIOGETINFO, &fbinfo);

    display_t *display = calloc(1, sizeof(display_t));
    
    display->width = fbinfo.width;
    display->height = fbinfo.height;
    display->fd = fd;
    display->state = mmap(NULL, fbinfo.width*fbinfo.height*4, PROT_READ | PROT_WRITE, 0, fd, 0);

    return display;
}

void
display_close(display_t *display)
{
    close(display->fd);
    free(display);
}

int
display_width(display_t *display)
{
    return display->width;
}

int
display_height(display_t *display)
{
    return display->height;
}

void
display_render(display_t *display, canvas_t *canvas)
{
    memcpy(display->state, canvas->pixels, display->width*display->height*4);
}
