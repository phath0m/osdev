/*
 * this file is responsible managing window objects, which represent an
 * actual physical window displayed on the screen
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <collections/list.h>
#include <libmemgfx/canvas.h>
#include <sys/mman.h>
#include "window.h"

/* for some reason newlib isn't putting this prototype inside unistd.h */
extern int ftruncate(int fd, off_t length); 

bool
window_boundcheck(struct window *window, int x, int y)
{
    int max_x = window->x + window->width;
    int max_y = window->y + window->height;
    return x >= window->x && x < max_x && y >= window->y && y < max_y;
}

struct window *
window_new(int x, int y, int width, int height)
{
    /* used to generate unique names for each window*/
    static int win_counter = 0;

    struct window *window = calloc(1, sizeof(struct window));

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;

    /* 
     * unique name for shm object that can be used to expose the actual raw
     * canvas to other programs for faster and more efficient drawing
     */
    sprintf(window->shm_name, "/xtc_win_%d", win_counter++);

    int memfd = shm_open(window->shm_name, O_RDWR | O_CREAT, 0700);
   
    ftruncate(memfd, width*height*4); 

    color_t *pixels = mmap(NULL, width*height*4, PROT_READ | PROT_WRITE,
            MAP_SHARED, memfd, 0);

    window->canvas = canvas_from_mem(width, height, CANVAS_DOUBLE_BUFFER, pixels);

    strcpy(window->title, "Form 1");

    return window;
}

void
window_set_title(struct window *win, const char *title)
{
    strncpy(win->title, title, sizeof(win->title));
}

void
window_draw(struct window *win, canvas_t *canvas)
{
    canvas_fill(canvas, win->x, win->y, win->width + 1, 20, WINDOW_TITLE_COLOR);
    canvas_rect(canvas, win->x, win->y, win->width + 1, win->height + 20, WINDOW_BORDER_COLOR);

    int text_width = strlen(win->title) * 10;

    int center_x = win->width / 2 - text_width / 2;

    canvas_puts(canvas, win->x + center_x, win->y + 4, win->title, 0xFFFFFF);
    canvas_putcanvas(canvas, win->x + 1, win->y + 20, win->canvas);
}

void
window_click(struct window *self, struct click_event *event)
{
    int relative_x = event->x - self->x;
    int relative_y = event->y - self->y;

    if (relative_y < 20) {
        self->dragged = 1;
        return;
    }
  
    if (LIST_SIZE(&self->events) > 5) {
        return;
    }

    struct window_event *winevent = calloc(1, sizeof(struct window_event));

    winevent->type = WINEVENT_CLICK;
    winevent->x = relative_x;
    winevent->y = relative_y;
 
    window_post_event(self, winevent);
}

void
window_keypress(struct window *self, uint8_t scancode)
{
    struct window_event *winevent = calloc(1, sizeof(struct window_event));

    winevent->type = WINEVENT_KEYPRESS;
    winevent->x = scancode;
    window_post_event(self, winevent);
}

void
window_hover(struct window *self, struct click_event *event)
{
}

void
window_post_event(struct window *self, struct window_event *event)
{
    thread_spin_lock(&self->event_lock);
    
    list_append(&self->events, event);

    thread_spin_unlock(&self->event_lock);
    
    thread_cond_signal(&self->event_cond);
}
