#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "canvas.h"
#include "display.h"
#include "utils.h"

display_t *
display_open()
{
    int fd;
    display_t *display;

    struct lfb_info fbinfo;


    fd = open("/dev/lfb", O_RDWR);

    if (fd == -1) {
        return NULL;
    }

    /* First; reset the textscreen (This ensures kernel output will be visible if panic) */
    ioctl(fd, TXIORST, NULL);

    /* Now get the framebuffer dimensions */
    ioctl(fd, FBIOGETINFO, &fbinfo);

    display = calloc(1, sizeof(display_t));
    
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
display_render(display_t *display, canvas_t *canvas, int x, int y, int width, int height)
{
    int max_height;
    int max_width;
    int max_y;
    int min_width;
    int min_height;

    color_t *fb;
    color_t *srcpixels;
    pixbuf_t *pixbuf;

    pixbuf = canvas->frontbuffer;
    srcpixels = pixbuf->pixels;
    fb = display->state;

    max_width = width;
    max_height = height;
    min_width = display->width - x;
    min_height = display->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    max_y = y + height;

    while (y < max_y) {
        void *dst;
        void *src;

        dst = &fb[display->width * y + x];
        src = &srcpixels[canvas->width * y + x];
        
        fast_memcpy_d(dst, src, width*4);
        y++;
    }
}
