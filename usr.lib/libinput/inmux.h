#ifndef _LIBINPUT_INMUX_T
#define _LIBINPUT_INMUX_T

#include "kbd.h"
#include "mouse.h"

#define INMUX_KBD       0x01
#define INMUX_MOUSE     0x02

typedef struct inmux {
    mouse_t *   mouse;
    kbd_t   *   kbd;
} inmux_t;

typedef struct inevent {
    int     type;
    union {
        struct kbd_event    kbd_event;
        struct mouse_event  mouse_event;
    } un;
} inevent_t;

inmux_t *inmux_open();
void inmux_close(inmux_t *inmux);
void inmux_next_event(inmux_t *inmux, struct inevent *eventbuf);

#endif
