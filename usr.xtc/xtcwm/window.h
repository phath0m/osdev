#ifndef _WINDOW_H
#define _WINDOW_H

#include <stdbool.h>
#include <thread.h>
#include <collections/list.h>
#include <libmemgfx/canvas.h>


#define WIN_ICONIFIED           0x01
#define WIN_FOCUSED             0x02
#define WIN_DRAGGED             0x04
#define WIN_RESIZED             0x08

#define WINEVENT_CLICK          0x01
#define WINEVENT_KEYPRESS       0x02
#define WINEVENT_RESIZE         0x03

struct click_event {
    int     x;
    int     y;
    int     prev_x;
    int     prev_y;
};

struct window {
    int             active;
    int             redraw;
    int             x;
    int             y;
    int             state;
    int             resize_width;
    int             resize_height;
    int             prev_x;
    int             prev_y;
    int             restore_x;
    int             restore_y;
    int             absolute_height;
    int             absolute_width;
    int             height;
    int             width;
    int             prev_height;
    int             prev_width;
    int             mouse_down;
    char            shm_name[256];
    char            title[256];
    canvas_t *      canvas;
    pixbuf_t *      pixbuf;
    int             pixbuf_size;
    struct list     events;
    thread_spinlock_t    event_lock;
    thread_cond_t   event_cond;  
};

struct window_event {
    int             type;
    int             x;
    int             y;
    int             state;
};

#define IS_WINDOW_DRAGGED(w) (((w)->state & WIN_DRAGGED) == WIN_DRAGGED)
#define IS_WINDOW_RESIZED(w) (((w)->state & WIN_RESIZED) == WIN_RESIZED)
#define IS_WINDOW_FOCUSED(w) (((w)->state & WIN_FOCUSED) == WIN_FOCUSED)

void add_new_window(void *ctx, struct window *window);

struct window * window_new(int x, int y, int width, int height);
void window_destroy(struct window *window);
void window_set_title(struct window *win, const char *title);
bool window_boundcheck(struct window *window, int x, int y);
void window_draw(struct window *window, canvas_t *canvas);
void window_click(struct window *self, struct click_event *event);
void window_hover(struct window *self, struct click_event *event);
void window_keypress(struct window *self, uint8_t scancode);
void window_request_resize(struct window *self, int width, int height);
void window_resize(struct window *self, int fillcolor, int width, int height);
void window_post_event(struct window *self, struct window_event *event);

#endif
