#include <stdlib.h>
#include <sys/ioctl.h>
#include "kbd.h"
#include "inmux.h"
#include "mouse.h"

inmux_t *
inmux_open()
{
    inmux_t *inmux = calloc(1, sizeof(inmux_t));
    inmux->mouse = mouse_open();
    inmux->kbd = kbd_open();

    return inmux;
}

void
inmux_close(inmux_t *inmux)
{
    kbd_close(inmux->kbd);
    mouse_close(inmux->mouse);
    free(inmux);
}

void
inmux_next_event(inmux_t *inmux, struct inevent *eventbuf)
{
    int keys_available = 0;

    kbd_t *kbd = inmux->kbd;
    mouse_t *mouse = inmux->mouse;


    ioctl(kbd->fd, FIONREAD, &keys_available);
    
    if (keys_available > 0) {
        kbd_next_event(kbd, &eventbuf->un.kbd_event);
        eventbuf->type = INMUX_KBD;
    } else {
        mouse_next_event(mouse, &eventbuf->un.mouse_event);
        eventbuf->type = INMUX_MOUSE;
    }


}
