#ifndef _LIBMEMGFX_DISPLAY_H
#define _LIBMEMGFX_DISPLAY_H


typedef struct display {
    int     fd;
    int     width;
    int     height;
    void *  state;
} display_t;


display_t *display_open();
int display_width(display_t *display);
int display_height(display_t *display);
void display_close(display_t *display);
void display_render(display_t *display, canvas_t *canvas);

#endif
