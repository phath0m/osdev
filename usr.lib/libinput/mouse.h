#ifndef _LIBINPUT_MOUSE_H
#define _LIBINPUT_MOUSE_H

typedef struct mouse {
    int fd;
    int prev_x;
    int prev_y;

} mouse_t;

typedef enum {
    MOUSE_LEFT_CLICK,
    MOUSE_RIGHT_CLICK,
    MOUSE_MOVE,
    MOUSE_NOP
} mouse_event_t;

struct mouse_event {
    mouse_event_t   event;
    int             x;
    int             y;
};

mouse_t *mouse_open();
void mouse_close(mouse_t *mouse);
void mouse_next_event(mouse_t *mouse, struct mouse_event *eventbuf);

#endif
