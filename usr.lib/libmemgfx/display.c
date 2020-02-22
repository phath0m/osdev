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
    
    int fd = open("/dev/lfb", O_RDWR);

    if (fd == -1) {
        return NULL;
    }

    /* First; reset the textscreen (This ensures kernel output will be visible if panic) */
    ioctl(fd, TXIORST, NULL);

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
display_render(display_t *display, canvas_t *canvas, int x, int y, int width, int height)
{
    pixbuf_t *pixbuf = canvas->frontbuffer;

    color_t *srcpixels = pixbuf->pixels;
   
    color_t *fb = (color_t*)display->state;

    int max_width = width;
    int max_height = height;
    int min_width = display->width - x;
    int min_height = display->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    int max_y = y + height;

    while (y < max_y) {
        void *dst = &fb[display->width * y + x];
        void *src = &srcpixels[canvas->width * y + x];
        
        fast_memcpy_d(dst, src, width*4);
        y++;
    }
}
