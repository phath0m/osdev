#ifndef _WINDOW_H
#define _WINDOW_H

#include <stdbool.h>
#include <thread.h>
#include <collections/list.h>
#include <libmemgfx/canvas.h>


#define WINEVENT_CLICK          0x01
#define WINEVENT_KEYPRESS       0x02

#define WINDOW_BORDER_COLOR             0x3e868a
#define WINDOW_TITLE_COLOR              0x303030
#define WINDOW_ACTIVE_TITLE_COLOR       0x1f5770
#define WINDOW_ACTIVE_TEXT_COLOR        0xFFFFFF
#define WINDOW_TEXT_COLOR               0x5c5b5b
#define WINDOW_COLOR                    0xFFFFFF

#define CONTROL_BUTTON      0x01

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
    int             prev_x;
    int             prev_y;
    int             absolute_height;
    int             absolute_width;
    int             height;
    int             width;
    int             dragged;
    char            shm_name[256];
    char            title[256];
    canvas_t *      canvas;
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


void add_new_window(void *ctx, struct window *window);

struct window * window_new(int x, int y, int width, int height);
void window_destroy(struct window *window);
void window_set_title(struct window *win, const char *title);
bool window_boundcheck(struct window *window, int x, int y);
void window_draw(struct window *window, canvas_t *canvas);
void window_click(struct window *self, struct click_event *event);
void window_hover(struct window *self, struct click_event *event);
void window_keypress(struct window *self, uint8_t scancode);
void window_post_event(struct window *self, struct window_event *event);

#endif
