#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "mouse.h"


mouse_t *
mouse_open()
{
    int fd;

    mouse_t *mouse;

    fd = open("/dev/mouse", O_RDONLY);

    if (fd == -1) {
        return NULL;
    }

    mouse = calloc(1, sizeof(mouse_t));

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
    int buttons;
    int x;
    int y;

    mouse_event_t event;

    uint8_t mouse_state[3];

    if (read(mouse->fd, mouse_state, 3) != 3) {
        eventbuf->event = MOUSE_NOP;
        return;
    }

    buttons = mouse_state[0];

    if ((buttons & 0xC0)) {
        /* apparently this indicates an overflow with the X and Y values */
        /* we will ignore the packet if this is set */
        eventbuf->event = MOUSE_NOP;
        eventbuf->x = 0;
        eventbuf->y = 0;
        return;
    }

    x = mouse_state[1];
    y = mouse_state[2];

    if ((buttons & 0x20)) {
        x = -x;
    }

    if ((buttons & 0x10)) {
        y = -y;
    }

    event = MOUSE_NOP;

    if (x != mouse->prev_x || y != mouse->prev_y) {
        eventbuf->x = (int)x;
        eventbuf->y = (int)y;
        event = MOUSE_MOVE;

        mouse->prev_x = x;
        mouse->prev_y = y;
    } else {
        eventbuf->x = (int)x;
        eventbuf->y = (int)y;
    }

    if ((buttons & 0x01)) {
        eventbuf->event = MOUSE_LEFT_CLICK;
        event = MOUSE_LEFT_CLICK;
    }

    eventbuf->event = event;
}
