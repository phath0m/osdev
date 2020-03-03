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
    int max_x = window->x + window->absolute_width;
    int max_y = window->y + window->absolute_height;
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
    window->prev_x = x;
    window->prev_y = y;
    window->width = width;
    window->height = height;
    window->absolute_width = width + 2;
    window->absolute_height = height + 21;

    window->redraw = 1;

    /* 
     * unique name for shm object that can be used to expose the actual raw
     * canvas to other programs for faster and more efficient drawing
     */
    sprintf(window->shm_name, "/xtc_win_%d", win_counter++);

    int memfd = shm_open(window->shm_name, O_RDWR | O_CREAT, 0700);
   
    size_t pixbuf_size = sizeof(pixbuf_t)+(width*height*4);
    ftruncate(memfd, pixbuf_size); 

    pixbuf_t *pixbuf = mmap(NULL, pixbuf_size, PROT_READ | PROT_WRITE,
            MAP_SHARED, memfd, 0);

    memset(pixbuf, 0, pixbuf_size);

    window->canvas = canvas_from_mem(width, height, 0, pixbuf);

    strcpy(window->title, "Form 1");

    return window;
}

void
window_destroy(struct window *window)
{
}

void
window_set_title(struct window *win, const char *title)
{
    strncpy(win->title, title, sizeof(win->title));
    win->redraw = 1;
}

void
window_draw(struct window *win, canvas_t *canvas)
{
    if (win->redraw) {
        
        int title_color = WINDOW_TITLE_COLOR;
        int text_color = WINDOW_TEXT_COLOR;
        int chisel_color_dark = CHISEL_INACTIVE_DARK;
        int chisel_color_light = CHISEL_INACTIVE_LIGHT;

        if (win->active) {
            title_color = WINDOW_ACTIVE_TITLE_COLOR;
            text_color = WINDOW_ACTIVE_TEXT_COLOR;
            chisel_color_dark = CHISEL_ACTIVE_DARK;
            chisel_color_light = CHISEL_ACTIVE_LIGHT;
        }

        canvas_fill(canvas, win->x + 1, win->y, win->width - 1, 19, title_color);
        canvas_fill(canvas, win->x + 2, win->y + 1, win->width - 2, 1, chisel_color_light);
        canvas_fill(canvas, win->x + 2, win->y + 18, win->width - 2, 1, chisel_color_dark);
        canvas_fill(canvas, win->x + 1, win->y + 1, 1, 19, chisel_color_light);
        canvas_fill(canvas, win->x + win->width - 1, win->y + 1, 1, 19, chisel_color_dark); 
        canvas_rect(canvas, win->x, win->y, win->width, 19, WINDOW_BORDER_COLOR);
        canvas_rect(canvas, win->x, win->y, win->width, win->height + 20, WINDOW_BORDER_COLOR);

        int text_width = strlen(win->title) * 10;

        int center_x = win->width / 2 - text_width / 2;

        canvas_puts(canvas, win->x + center_x, win->y + 4, win->title, text_color);
        canvas_putcanvas(canvas, win->x + 1, win->y + 20, 0, 0, 
                win->width, win->height, win->canvas);

        canvas_invalidate_region(canvas, win->prev_x, win->prev_y,
                win->absolute_width,
                win->absolute_height);


        win->redraw = 0;
    }

    pixbuf_t *buf = win->canvas->pixels;

    if (buf->dirty) {
        
        int width = buf->max_x - buf->min_x;
        int height = buf->max_y - buf->min_y;

        canvas_putcanvas(canvas, win->x + 1 + buf->min_x, win->y + 20 + buf->min_y, buf->min_x,
                buf->min_y, width, height, win->canvas);
        buf->dirty = 0;
    }
}

void
window_click(struct window *self, struct click_event *event)
{
    int relative_y = event->y - self->y;

    if (relative_y < 20) {
        self->dragged = 1;
        self->prev_x = self->x;
        self->prev_y = self->y;
        return;
    }
  
    if (LIST_SIZE(&self->events) > 5) {
        return;
    }

    self->mouse_down = true;
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
    if (self->mouse_down) {
        int relative_x = event->x - self->x;
        int relative_y = event->y - self->y;

        struct window_event *winevent = calloc(1, sizeof(struct window_event));
    
        winevent->type = WINEVENT_CLICK;
        winevent->x = relative_x - 1;
        winevent->y = relative_y - 21;

        window_post_event(self, winevent);

        self->mouse_down = false;
    }
}

void
window_post_event(struct window *self, struct window_event *event)
{
    thread_spin_lock(&self->event_lock);
    
    list_append(&self->events, event);

    thread_spin_unlock(&self->event_lock);
    
    thread_cond_signal(&self->event_cond);
}
