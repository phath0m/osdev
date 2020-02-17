/*
 * So this is pretty crappy but it is mainly to servce as a proof of
 * concept. As the kernel and supporting runtime is tweaked, I'll 
 * probably end up rewriting this. 
 *
 * This file is primarily responsible for rendering what's on the
 * screen and receiving actual input. 
 *
 * window.c is responsible for handling the actual events and stuff
 * that are sent to individuals windows
 *
 * server.c is responsible for handling communication with the client
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <unistd.h>
#include <xtcprotocol.h>
#include <collections/list.h>
#include <libinput/inmux.h>
#include <libmemgfx/canvas.h>
#include <libmemgfx/display.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "server.h"
#include "window.h"

/* disgusting global state */
int mouse_x = 0;
int mouse_y = 0;
int mouse_delta_x = 0;
int mouse_delta_y = 0;
int mouse_prev_x = 0;
int mouse_prev_y = 0;

static struct list windows;
static struct window *focused_window = NULL;

/* inefficiently draws all the windows */
static void
display_windows(canvas_t *canvas)
{
    list_iter_t iter;

    list_get_iter(&windows, &iter);

    void *win = NULL;

    while (iter_move_next(&iter, &win)) {
        window_draw(win, canvas);
    }
}

void
add_new_window(struct window *window)
{
    list_append(&windows, window);
}

/* dispatch actual mouse events to the window */
static void
handle_click(struct click_event *event)
{
    if (focused_window && focused_window->dragged) {
        focused_window->x = event->x;
        focused_window->y = event->y;
        return;
    }

    list_iter_t iter;

    list_get_iter(&windows, &iter);

    void *win = NULL;
    struct window *active_win = NULL;

    while (iter_move_next(&iter, &win)) {
        if (window_boundcheck(win, event->x, event->y)) {
            active_win = win;
        }
    }

    if (active_win) {
        window_click(active_win, event);
        
        if (active_win != win) {
            list_remove(&windows, active_win);
            list_append(&windows, active_win);
        }
        
        focused_window = active_win;
    }
}

/* handle hover events [nothing actually uses this lol] */
static void
handle_hover(struct click_event *event)
{
    list_iter_t iter;

    list_get_iter(&windows, &iter);

    void *win = NULL;
    struct window *active_win = NULL;

    while (iter_move_next(&iter, &win)) {
        if (window_boundcheck(win, event->x, event->y)) {
            active_win = win;
        }
    }

    if (active_win) {
        active_win->dragged = 0;
        window_hover(active_win, event);
    }
}

/* handle key event */
static void
handle_key_event(struct kbd_event *eventbuf)
{
    window_keypress(focused_window, eventbuf->scancode);
}

/* actually figure out wtf the mouse is doing */
static void
handle_mouse_event(struct mouse_event *eventbuf)
{
    switch (eventbuf->event) {
        case MOUSE_LEFT_CLICK:
        case MOUSE_RIGHT_CLICK:
        case MOUSE_MOVE:
            mouse_delta_x = eventbuf->x;
            mouse_delta_y = eventbuf->y;
            break;
        case MOUSE_NOP:
            return;
        default:
            break;
    }

    struct click_event event;
    event.x = mouse_x;
    event.y = mouse_y;
    event.prev_x = mouse_prev_x;
    event.prev_y = mouse_prev_y;

    switch (eventbuf->event) {
        case MOUSE_MOVE:
            handle_hover(&event);
            break;
        case MOUSE_LEFT_CLICK:
            handle_click(&event);
            break;
        default:
            break;
    }
    
    mouse_prev_x = mouse_x;
    mouse_prev_y = mouse_y;
}

/* thread responsible for reading from both the keyboard and the mouse */
static void *
io_loop(void *arg)
{
    inmux_t *inmux = inmux_open();

    for (;;) {
        struct inevent event;

        inmux_next_event(inmux, &event);

        switch (event.type) {
            case INMUX_KBD:
                handle_key_event(&event.un.kbd_event);
            case INMUX_MOUSE:
                handle_mouse_event(&event.un.mouse_event);
                break;
        }
    }

    inmux_close(inmux);

    return NULL;
}

/* thread responsible for rendering the screen, roughly 120 times a second */
static void *
display_loop(void *unused)
{
    display_t *display = display_open();

    int width = display_width(display);
    int height = display_height(display);

    int tick = 0;

    canvas_t *canvas = canvas_new(width, height, 0);

    for (;;) {
        if (mouse_delta_x != 0) {
            mouse_prev_x = mouse_x;
        }

        if (mouse_delta_y != 0) {
            mouse_prev_y = mouse_y;
        }

        mouse_x += mouse_delta_x;
        mouse_y -= mouse_delta_y;

        if (mouse_x > 790) {
            mouse_x = 790;
        }

        if (mouse_y > 590) {
            mouse_y = 590;
        }

        if (mouse_x < 0) {
            mouse_x = 0;
        }

        if (mouse_y < 0) {
            mouse_y = 0;
        }

        if (tick == 8) {
            canvas_clear(canvas, 0x777777);
            display_windows(canvas);
            canvas_putc(canvas, mouse_x, mouse_y, 'x', 0x0);
            display_render(display, canvas);
            tick = 0;
        } else {
            tick++;
        }

        /* 
         * this is something that is in my libc but not an official function
         * it sleeps for a millisecond. 
         * We're not good enough for usleep() 
         */
        extern void msleep(int milliseconds);

        msleep(1);
    }

    canvas_destroy(canvas);
    display_close(display);

    return NULL;
}

int
main(int argc, const char *argv[])
{

    /* 
     * First, set the output to the serial device
     * 
     * this is useful for debugging
     */ 
    int serial = open("/dev/ttyS0", O_RDWR);

    for (int i = 0; i < 3; i++) {
        dup2(serial, i);
    }

    thread_t input_thread;
    thread_t display_thread;
    thread_create(&input_thread, io_loop, NULL);
    thread_create(&display_thread, display_loop, NULL);

    server_listen();

    return 0;
}
