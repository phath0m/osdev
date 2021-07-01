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
#include "property.h"
#include "window.h"
#include "wm.h"


#define WINDOW_CLIENT_X     6
#define WINDOW_CLIENT_Y     27

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
    window->absolute_width = width + 12;
    window->absolute_height = height + 33;

    window->redraw = 1;

    /* 
     * unique name for shm object that can be used to expose the actual raw
     * canvas to other programs for faster and more efficient drawing
     */
    sprintf(window->shm_name, "/xtc_win_%d", win_counter++);

    int memfd = shm_open(window->shm_name, O_RDWR | O_CREAT, 0700);

    int screen_width = XTC_GET_PROPERTY(XTC_SCREEN_WIDTH);
    int screen_height = XTC_GET_PROPERTY(XTC_SCREEN_HEIGHT);

    size_t pixbuf_size = sizeof(pixbuf_t)+(screen_width*screen_height*4);
    ftruncate(memfd, pixbuf_size); 

    pixbuf_t *pixbuf = mmap(NULL, pixbuf_size, PROT_READ | PROT_WRITE,
            MAP_SHARED, memfd, 0);

    memset(pixbuf, 0, pixbuf_size);

    window->canvas = canvas_from_mem(width, height, 0, pixbuf);
    window->pixbuf = pixbuf;
    window->pixbuf_size = pixbuf_size;

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
    bool iconified = (win->state & WIN_ICONIFIED) == WIN_ICONIFIED;
    bool focused = (win->state & WIN_FOCUSED) == WIN_FOCUSED;

    if (win->redraw) {
        int title_color = XTC_GET_PROPERTY(XTC_WIN_TITLE_COL);
        int text_color = XTC_GET_PROPERTY(XTC_WIN_TEXT_COL);
        int chisel_color_dark = XTC_GET_PROPERTY(XTC_CHISEL_DARK_COL);
        int chisel_color_light = XTC_GET_PROPERTY(XTC_CHISEL_LIGHT_COL);

        if (focused) {
            title_color = XTC_GET_PROPERTY(XTC_WIN_ACTIVE_TITLE_COL);
            text_color = XTC_GET_PROPERTY(XTC_WIN_ACTIVE_TEXT_COL);
            chisel_color_dark = XTC_GET_PROPERTY(XTC_CHISEL_ACTIVE_DARK_COL);
            chisel_color_light = XTC_GET_PROPERTY(XTC_CHISEL_ACTIVE_LIGHT_COL);
        }

        canvas_fill(canvas, win->x, win->y, win->absolute_width, win->absolute_height, title_color);
        canvas_rect(canvas, win->x, win->y, win->absolute_width, win->absolute_height, chisel_color_light);

        canvas_rect(canvas, win->x, win->y + win->absolute_height - 1, win->absolute_width, 1, chisel_color_dark);
        canvas_rect(canvas, win->x + win->absolute_width - 1, win->y, 1, win->absolute_height, chisel_color_dark);
        canvas_fill(canvas, win->x + 5, win->y + win->absolute_height - 5, win->absolute_width - 10, 1, chisel_color_light);
        canvas_fill(canvas, win->x + win->absolute_width - 5, win->y + 5, 1, win->absolute_height - 10, chisel_color_light);
        canvas_fill(canvas, win->x + 5, win->y + 5, win->absolute_width - 10, 1, chisel_color_dark);
        canvas_fill(canvas, win->x + 5, win->y + 5, 1, win->absolute_height - 10, chisel_color_dark);
        //canvas_rect(canvas, win->x, win->y, win->width, win->height, chisel_color_light);

        //canvas_fill(canvas, win->x + 4, win->y, win->width - 4, 19, title_color);
        //canvas_fill(canvas, win->x + 2, win->y + 1, win->width - 1, 1, chisel_color_light);
        //canvas_fill(canvas, win->x + 2, win->y + 18, win->width - 1, 1, chisel_color_dark);
        //canvas_fill(canvas, win->x + 1, win->y + 1, 1, 19, chisel_color_light);
        //canvas_fill(canvas, win->x + win->width, win->y + 1, 1, 19, chisel_color_dark); 
        //canvas_rect(canvas, win->x, win->y, win->width + 1, 19, XTC_GET_PROPERTY(XTC_WIN_BORDER_COL));
        //canvas_rect(canvas, win->x, win->y, win->width + 1, win->height + 20, XTC_GET_PROPERTY(XTC_WIN_BORDER_COL));

        if (!iconified) {
            // iconify button
            canvas_rect(canvas, win->x + 6, win->y + 6, 19, 19, chisel_color_dark);
            canvas_fill(canvas, win->x + 6, win->y + 25, 19, 1, chisel_color_light);
            canvas_fill(canvas, win->x + 6, win->y + 6, 1, 19, chisel_color_light);
            canvas_fill(canvas, win->x + 10, win->y + 14, 12, 4, chisel_color_dark);
            canvas_fill(canvas, win->x + 6, win->y + 25, win->absolute_width - 12, 1, chisel_color_light);
            canvas_fill(canvas, win->x + 6, win->y + 26, win->absolute_width - 12, 1, chisel_color_dark);

            canvas_fill(canvas, win->x, win->y + 25, 5, 1, chisel_color_dark);
            canvas_fill(canvas, win->x, win->y + 26, 5, 1, chisel_color_light);

            canvas_fill(canvas, win->x + win->absolute_width - 5, win->y + 26, 5, 1, chisel_color_dark);
            canvas_fill(canvas, win->x + win->absolute_width - 5, win->y + 25, 5, 1, chisel_color_light);

            canvas_fill(canvas, win->x + 25, win->y, 1, 5, chisel_color_dark);
            canvas_fill(canvas, win->x + 26, win->y, 1, 5, chisel_color_light);

            canvas_fill(canvas, win->x + win->absolute_width - 25, win->y, 1, 5, chisel_color_light);
            canvas_fill(canvas, win->x + win->absolute_width - 26, win->y, 1, 5, chisel_color_dark);


            canvas_fill(canvas, win->x, win->y + win->absolute_height - 25, 5, 1, chisel_color_dark);
            canvas_fill(canvas, win->x, win->y + win->absolute_height - 26, 5, 1, chisel_color_light);

            canvas_fill(canvas, win->x + win->absolute_width - 5, win->y + win->absolute_height - 25, 5, 1, chisel_color_light);
            canvas_fill(canvas, win->x + win->absolute_width - 5, win->y + win->absolute_height - 26, 5, 1, chisel_color_dark);
            canvas_fill(canvas, win->x + 25, win->y + win->absolute_height - 5, 1, 5, chisel_color_dark);
            canvas_fill(canvas, win->x + 26, win->y + win->absolute_height - 5, 1, 5, chisel_color_light);

            canvas_fill(canvas, win->x + win->absolute_width - 26, win->y + win->absolute_height - 5, 1, 5, chisel_color_dark);
            canvas_fill(canvas, win->x + win->absolute_width - 25, win->y + win->absolute_height - 5, 1, 5, chisel_color_light);


            //canvas_rect(canvas, win->x + 4, win->y + 3, 12, 12, text_color);
            //canvas_fill(canvas, win->x + 7, win->y + 11, 7, 2, text_color);

            // maximize button
            //canvas_rect(canvas, win->x + win->width - 16, win->y + 3, 12, 12, text_color);
            //canvas_puts(canvas, win->x + win->width - 15, win->y + 3, "x", 0x0);

            //canvas_rect(canvas, win->x + win->width - 16, win->y + 3, 8, 8, text_color);
            //canvas_rect(canvas, win->x + win->width - 16, win->y + 3, 5, 5, text_color);
        }

        int text_width = strlen(win->title) * 10;

        int center_x = win->width / 2 - text_width / 2;

        canvas_puts(canvas, win->x + center_x, win->y + 10, win->title, text_color);

        if (!iconified) {
            canvas_putcanvas(canvas, win->x + 6, win->y + 27, 0, 0, 
                    win->width, win->height, win->canvas);

            canvas_invalidate_region(canvas, win->prev_x, win->prev_y,
                    win->absolute_width,
                    win->absolute_height);
        }

        win->redraw = 0;
    }

    pixbuf_t *buf = win->canvas->pixels;

    if (buf->dirty && !iconified) {
        
        int width = buf->max_x - buf->min_x;
        int height = buf->max_y - buf->min_y;

        canvas_putcanvas(canvas, win->x + 6 + buf->min_x, win->y + 27 + buf->min_y, buf->min_x,
                buf->min_y, width, height, win->canvas);
        buf->dirty = 0;
    }
}

void
window_click(struct window *self, struct click_event *event)
{
    int relative_y = event->y - self->y;
    int relative_x = event->x - self->x;

    if (relative_y < 27 && relative_x > 27) {
        self->state |= WIN_DRAGGED;
        self->prev_x = self->x;
        self->prev_y = self->y;
        return;
    }
 
    if (relative_y > (self->absolute_height - 10)) {
        self->state |= WIN_RESIZED;
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

    	if ((self->state & WIN_ICONIFIED)) {
        	self->state &= ~WIN_ICONIFIED;
            self->width = self->prev_width;
            self->height = self->prev_height;
            self->absolute_height = 21 + self->height;
            self->absolute_width = self->width + 2;
            self->x = self->restore_x;
            self->y = self->restore_y;
            wm_redraw();
    	} else if (relative_y < 20 && relative_x < 20) {
        	self->state |= WIN_ICONIFIED;
            self->prev_width = self->width;
            self->prev_height = self->height;
            self->restore_x = self->x;
            self->restore_y = self->y;
            self->absolute_height = 21;
            self->height = 0;
            self->width = strlen(self->title)*12 + 8;
            self->absolute_width = self->width + 2;
            wm_redraw();
	    } else if (relative_y > 20) {
            struct window_event *winevent = calloc(1, sizeof(struct window_event));
        
            winevent->type = WINEVENT_CLICK;
            winevent->x = relative_x - 1;
            winevent->y = relative_y - 21;

            window_post_event(self, winevent);
		}

        self->mouse_down = false;
    }
}

void
window_resize(struct window *self, int color, int width, int height)
{
    self->width = width;
    self->height = height;
    self->absolute_width = width + 12;
    self->absolute_height = height + 33;
    //self->canvas->width = width;
    //self->canvas->height = height;
    //self->canvas->buffersize = width*height*4;
    canvas_resize(self->canvas, (color_t)color, width, height);
    self->redraw = 1;
}

void
window_request_resize(struct window *self, int width, int height)
{
    struct window_event *winevent = calloc(1, sizeof(struct window_event));
    winevent->type = WINEVENT_RESIZE;
    winevent->x = width;
    winevent->y = height;
    
    window_post_event(self, winevent);
}

void
window_post_event(struct window *self, struct window_event *event)
{
    thread_spin_lock(&self->event_lock);
    
    list_append(&self->events, event);

    thread_spin_unlock(&self->event_lock);
    thread_cond_signal(&self->event_cond);
}
