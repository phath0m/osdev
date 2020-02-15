#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "kbd.h"


kbd_t *
kbd_open()
{
    
    int fd = open("/dev/kbd", O_RDONLY);

    if (fd == -1) {
        return NULL;
    }

    kbd_t *kbd = calloc(1, sizeof(kbd_t));

    kbd->fd = fd;

    return kbd;
}

void
kbd_close(kbd_t *kbd)
{
    close(kbd->fd);
    free(kbd);
}

void
kbd_next_event(kbd_t *kbd, struct kbd_event *eventbuf)
{
    uint8_t scancode;

    read(kbd->fd, &scancode, 1);
    
    eventbuf->scancode = scancode;
}
