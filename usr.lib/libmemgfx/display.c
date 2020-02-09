#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "canvas.h"
#include "display.h"

/* right now I'm not providing mman.h because my
 * mmap is half baked and I don't want GNU autocruft
 * to think that I have it properly implemented
 */

extern void *mmap(void *addr, size_t length, int prot, int flags,
                int fd, off_t offset);

display_t *
display_open()
{
    
    int fd = open("/dev/lfb", O_RDWR);

    if (fd == -1) {
        return NULL;
    }

    display_t *display = calloc(1, sizeof(display_t));
    
    display->width = 800;
    display->height = 600;
    display->fd = fd;
    display->state = mmap(NULL, 800*600*4, 7, 0, fd, 0);

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
    return 800;
}

int
display_height(display_t *display)
{
    return 600;
}

void
display_render(display_t *display, canvas_t *canvas)
{
    memcpy(display->state, canvas->pixels, 800*600*4);
}
