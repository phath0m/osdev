#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mouse.h"


mouse_t *
mouse_open()
{
    
    int fd = open("/dev/mouse", O_RDONLY);

    if (fd == -1) {
        return NULL;
    }

    mouse_t *mouse = calloc(1, sizeof(mouse_t));

    mouse->fd = fd;

    return mouse;
}

void
mouse_close(mouse_t *mouse)
{
    close(mouse->fd);
    free(mouse);
}

void
mouse_next_event(mouse_t *mouse, struct mouse_event *eventbuf)
{
    char mouse_state[3];

    if (read(mouse->fd, mouse_state, 3) != 3) {
        eventbuf->event = MOUSE_NOP;
        return;
    }

    int buttons = mouse_state[0];

    if ((buttons & 0xC0)) {
        /* apparently this indicates an overflow with the X and Y values */
        /* we will ignore the packet if this is set */
        eventbuf->event = MOUSE_NOP;
        return;
    }

    unsigned int x = mouse_state[1];
    unsigned int y = mouse_state[2];

    if ((buttons & 0x20)) {
        y |= 0xFFFFFF00;
    }

    if ((buttons & 0x10)) {
        x |= 0xFFFFFF00;
    }

    mouse_event_t event = MOUSE_NOP;

    if (x != mouse->prev_x || y != mouse->prev_y) {
        eventbuf->x = (int)x;
        eventbuf->y = (int)y;//mouse->prev_y + y;
        event = MOUSE_MOVE;

        mouse->prev_x = x;
        mouse->prev_y = y;
    } else {
        eventbuf->x = (int)x;//mouse->prev_x;
        eventbuf->y = (int)y;//mouse->prev_y;
    }

    if ((buttons & 0x01)) {
        eventbuf->event = MOUSE_LEFT_CLICK;
        event = MOUSE_LEFT_CLICK;
    }

    eventbuf->event = event;
}
