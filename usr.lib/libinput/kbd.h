#ifndef _LIBINPUT_KBD_H
#define _LIBINPUT_KBD_H

typedef struct kbd {
    int     fd;
} kbd_t;

typedef struct kbd_event {
    int     scancode;
} kbd_event_t;

kbd_t * kbd_open();

void kbd_close(kbd_t *kbd);

void kbd_next_event(kbd_t *kbd, struct kbd_event *eventbuf);

#endif
