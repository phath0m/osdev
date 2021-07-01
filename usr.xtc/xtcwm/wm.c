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

#include "property.h"
#include "server.h"
#include "window.h"
#include "wm.h"

// delete
static int actual_mouse_delta_x = 0;
static int actual_mouse_delta_y = 0;

int xtc_properties[256] = {
    0x505170,	/* background */	 
    0x000000, 	/* window border color (inactive) */
    0x000000, 	/* window title text color (inactive) */
    0xA0A0A0, 	/* window title color */
    0x000000, 	/* window title color (active) */
    0xFFFFFF, 	/* window text color (active) */
    0xFFFFFF, 	/* window body color */
    0xC0C0C0, 	/* window chisel color dark (inactive) */
    0xFFFFFF, 	/* window chisel color light (inactive) */
    0x404040, 	/* window chisel color dark (active) */
    0xA0A0A0 	/* window chisel color light (active) */
};

/* container for global state about the window manager */
struct wmctx xtc_context;

static int *cursor_icons[] = {
    /* normal cursor icon */
    (int[]) {
		2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
		2, 1, 1, 2, 0, 0, 0, 0, 0, 0,
		2, 1, 1, 1, 2, 0, 0, 0, 0, 0,
		2, 1, 1, 1, 1, 2, 0, 0, 0, 0,
		2, 1, 1, 1, 1, 1, 2, 0, 0, 0,
		2, 1, 1, 1, 1, 1, 1, 2, 0, 0,
		2, 1, 1, 1, 1, 1, 1, 1, 2, 0,
		2, 1, 1, 1, 1, 1, 1, 1, 1, 2,
		2, 1, 1, 1, 1, 1, 1, 2, 2, 2,
		2, 1, 1, 2, 1, 1, 2, 0, 0, 0,
		2, 1, 2, 0, 2, 1, 1, 2, 0, 0,
		2, 2, 0, 0, 0, 2, 1, 1, 2, 0,
		0, 0, 0, 0, 0, 2, 1, 1, 2, 0,
		0, 0, 0, 0, 0, 0, 2, 2, 0, 0,
    },
    /* window resize cursor icon */
    (int[]) {
        2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
        2, 1, 1, 1, 1, 2, 0, 0, 0, 0,
        2, 1, 2, 2, 2, 2, 0, 0, 0, 0,
        2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
        2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
        2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
        2, 1, 2, 0, 1, 1, 0, 0, 0, 0,
        2, 2, 2, 0, 1, 2, 0, 2, 2, 2,
        0, 0, 0, 0, 1, 2, 0, 2, 1, 2,
        0, 0, 1, 1, 1, 2, 0, 2, 1, 2,
        0, 0, 1, 2, 2, 2, 0, 2, 1, 2,
        0, 0, 0, 0, 0, 0, 0, 2, 1, 2,
        0, 0, 0, 0, 0, 0, 0, 2, 1, 2,
        0, 0, 0, 0, 2, 2, 2, 2, 1, 2,
        0, 0, 0, 0, 2, 1, 1, 1, 1, 2,
        0, 0, 0, 0, 2, 2, 2, 2, 2, 2,
    }
};

void
wm_add_window(struct window *win)
{
    list_append(&xtc_context.windows, win);
}

void
wm_remove_window(struct window *win)
{
    list_remove(&xtc_context.windows, win);
    wm_redraw();
}

/* mark a window as being active and trigger a re-draw */
static void
set_active_window(struct window *win)
{
    if (xtc_context.focused_window == win) {
        return;
    }

    if (xtc_context.focused_window) {
        xtc_context.focused_window->state &= ~WIN_FOCUSED;
        xtc_context.focused_window->redraw = 1;
    }

    xtc_context.focused_window = win;
    win->state |= WIN_FOCUSED;
    win->redraw = 1;
}

/* force re-draw of everything */
void
wm_redraw()
{
    xtc_context.redraw_flag = 1;

    list_iter_t iter;
    list_get_iter(&xtc_context.windows, &iter);

    struct window *win = NULL;

    while (iter_move_next(&iter, (void**)&win)) {
        win->redraw = 1;
    }
}

/* dispatch actual mouse events to the window */
static void
handle_click(struct click_event *event)
{
    if (xtc_context.focused_window && IS_WINDOW_DRAGGED(xtc_context.focused_window)) {
        xtc_context.focused_window->x = event->x;
        xtc_context.focused_window->y = event->y;
        return;
    }

    if (xtc_context.focused_window && IS_WINDOW_RESIZED(xtc_context.focused_window)) {
        xtc_context.focused_window->resize_width = event->x - xtc_context.focused_window->x;
        xtc_context.focused_window->resize_height = event->y - xtc_context.focused_window->y;
        return;
    }

    list_iter_t iter;
    list_get_iter(&xtc_context.windows, &iter);

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
            list_remove(&xtc_context.windows, active_win);
            list_append(&xtc_context.windows, active_win);
        }
      
        set_active_window(active_win); 

        if (IS_WINDOW_RESIZED(active_win)) {
            xtc_context.cursor_icon = CURSOR_ICON_RESIZE;
        }
    }
}

/* handle hover events */
static void
handle_hover(struct click_event *event)
{
    list_iter_t iter;
    list_get_iter(&xtc_context.windows, &iter);

    void *win = NULL;
    struct window *active_win = NULL;

    while (iter_move_next(&iter, &win)) {
        if (window_boundcheck(win, event->x, event->y)) {
            active_win = win;
        }
    }

    if (!active_win) {
        return;  
    }

    if (IS_WINDOW_DRAGGED(active_win)) {
        active_win->state &= ~WIN_DRAGGED;
        active_win->redraw = 1;
        wm_redraw();
    }

    if (IS_WINDOW_RESIZED(active_win)) {
        active_win->state &= ~WIN_RESIZED;
        active_win->redraw = 1;
        xtc_context.cursor_icon = CURSOR_ICON_NORMAL;
        window_request_resize(active_win, active_win->resize_width, active_win->resize_height);
        wm_redraw();
    }

    window_hover(active_win, event);
}

/* handle key event */
static void
handle_key_event(struct kbd_event *eventbuf)
{
    if (xtc_context.focused_window) {
        window_keypress(xtc_context.focused_window, eventbuf->scancode);
    }
}

/* actually figure out wtf the mouse is doing */
static void
handle_mouse_event(struct mouse_event *eventbuf)
{
    if (eventbuf->x == 1 || eventbuf->x == -1) {
        //eventbuf->x = 0;
    }

    if (eventbuf->y == 1 || eventbuf->y == -1) {
        //eventbuf->y = 0;
    }
    /*
    actual_mouse_delta_x = eventbuf->x;
    actual_mouse_delta_y = eventbuf->y;

    switch (eventbuf->event) {
        case MOUSE_LEFT_CLICK:
        case MOUSE_RIGHT_CLICK:
        case MOUSE_MOVE:
            
            xtc_context.mouse_delta_x = eventbuf->x;
            xtc_context.mouse_delta_y = eventbuf->y;
            break;
        case MOUSE_NOP:
            xtc_context.mouse_delta_x = eventbuf->x;
            xtc_context.mouse_delta_y = eventbuf->y;
            return;
        default:
            break;
    }*/

    if (eventbuf->x != 0 && eventbuf->x != actual_mouse_delta_x) {
        xtc_context.mouse_delta_x = eventbuf->x;
        actual_mouse_delta_x = eventbuf->x;
    }

    if (eventbuf->y != 0 && eventbuf->y != actual_mouse_delta_x) {
        xtc_context.mouse_delta_y = eventbuf->y;
        actual_mouse_delta_y = eventbuf->y;
    }

    struct click_event event;
    event.x = xtc_context.mouse_x;
    event.y = xtc_context.mouse_y;

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
}

/* thread responsible for reading from both the keyboard and the mouse */
static void *
io_loop(void *argp)
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

/* draw a wireframe box */
static void
draw_wireframe(canvas_t *canvas, int x, int y, int width, int height)
{
	int col = XTC_GET_PROPERTY(XTC_WIN_TITLE_COL);
    canvas_fill(canvas, x, y, width, 4, col);
    canvas_fill(canvas, x, y + height - 4, width, 4, col);
    canvas_fill(canvas, x, y, 4, height, col);
    canvas_fill(canvas, x + width - 4, y, 4, height, col);
}

/* draw all windows inefficiently */
static void
draw_windows(canvas_t *canvas)
{
    list_iter_t iter;
    list_get_iter(&xtc_context.windows, &iter);

    struct window *win = NULL;
    while (iter_move_next(&iter, (void**)&win)) {
        window_draw(win, canvas);
    }
}

/* actually render the wireframe on screen */
static void
render_wireframe(display_t *display, canvas_t *canvas, int x, int y, int width, int height)
{   
    display_render(display, canvas, x, y, width, 4);
    display_render(display, canvas, x, y + height - 4, width, 4);
    display_render(display, canvas, x, y, 4, height);
    display_render(display, canvas, x + width - 4, y, 4, height);
}

/* draw icon */
static void
draw_icon(canvas_t *canvas, int x, int y, int *icon)
{
    color_t pixels[10];

    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 10; px++) {
            int entry = icon[py*10+px];

            switch (entry) {
                case 0:
                    pixels[px] = 0xFFFFFFFF;
                    break;
                case 1:
                    pixels[px] = 0x00;
                    break;
                case 2:
                    pixels[px] = 0xFFFFFF;
                    break;
            }
        }
        canvas_putpixels(canvas, x, py + y, 10, 1, pixels);
    }
}

/* draw debug information */

static void
draw_debug_info(canvas_t *canvas)
{
    char debug_str[512];

    sprintf(debug_str, "X: %d Y: %d dX: %d dY: %d\n",
        xtc_context.mouse_x, xtc_context.mouse_y,
        xtc_context.mouse_delta_x, xtc_context.mouse_delta_y);

    canvas_fill(canvas, 0, 0, 400, 15, 0xFFFFFF);
    canvas_puts(canvas, 0, 0, debug_str, 0x00000);
}

/* thread responsible for rendering the screen, roughly 120 times a second */
static void *
display_loop(void *argp)
{
    display_t *display = display_open();

    int width = display_width(display);
    int height = display_height(display);

    XTC_SET_PROPERTY(XTC_SCREEN_WIDTH, width);
    XTC_SET_PROPERTY(XTC_SCREEN_HEIGHT, height);

    /* even though libmemgfx supports double buffering, we're not going to use it
     * so we can render the cursor on the frontbuffer only */
    canvas_t *backbuffer = canvas_new(width, height, CANVAS_PARTIAL_RENDER);
    canvas_t *frontbuffer = canvas_new(width, height, CANVAS_PARTIAL_RENDER);

    canvas_t *bg = NULL;
    char *wallpaper = getenv("WALLPAPER");

    if (wallpaper) {
        bg = canvas_from_targa(wallpaper, 0);
        canvas_scale(bg, width, height);
		unsetenv("WALLPAPER");
    }
    
    if (!bg) {
        bg = canvas_new(width, height, 0);
        canvas_clear(bg, XTC_GET_PROPERTY(XTC_BACKGROUND_COL));
    }    

    xtc_context.redraw_flag = 1;

    /* 
     * feels kind of dirty but here we will keep track of the past rendered
     * coordinates for the mouse so we can re-draw over the previous location
     *
     * This is ugly but doing it this way increases speed tenfold
     */
    int last_rendered_mouse_x = 0;
    int last_rendered_mouse_y = 0;
    
    /* last coordinates of wireframe box drawn when moving/resizing a window */
    int last_rendered_wf_x = 0;
    int last_rendered_wf_y = 0;
    int last_wf_width = 0;
    int last_wf_height = 0;

    int mouse_bounds_x = width - 8;
    int mouse_bounds_y = height - 12;

    for (;;) {
        if (xtc_context.redraw_flag) {
            canvas_putcanvas(backbuffer, 0, 0, 0, 0, bg->width, bg->height, bg);
            xtc_context.redraw_flag = 0;
        }

        draw_debug_info(backbuffer);

        xtc_context.mouse_x += xtc_context.mouse_delta_x;
        xtc_context.mouse_y -= xtc_context.mouse_delta_y;
        
        xtc_context.mouse_delta_x = 0;
        xtc_context.mouse_delta_y = 0;

        if (xtc_context.mouse_x > mouse_bounds_x) {
            xtc_context.mouse_x = mouse_bounds_x;
        }

        if (xtc_context.mouse_y > mouse_bounds_y) {
            xtc_context.mouse_y = mouse_bounds_y;
        }

        if (xtc_context.mouse_x < 0) {
            xtc_context.mouse_x = 0;
        }

        if (xtc_context.mouse_y < 0) {
            xtc_context. mouse_y = 0;
        }

        draw_windows(backbuffer);

        pixbuf_t *buf = backbuffer->pixels;
        
        int dirty_width = buf->max_x - buf->min_x;
        int dirty_height = buf->max_y - buf->min_y;

        canvas_putcanvas(frontbuffer, buf->min_x, buf->min_y, buf->min_x,
            buf->min_y, dirty_width, dirty_height, backbuffer);

        if (buf->dirty) { 
            display_render(display, frontbuffer, buf->min_x, buf->min_y, dirty_width, dirty_height);
        }

        if (last_rendered_mouse_x != xtc_context.mouse_x || last_rendered_mouse_y != xtc_context.mouse_y) {
            /* re-draw the cursor at the current mouse position */

            /* first; re-draw over the previous location */
            canvas_putcanvas(frontbuffer, last_rendered_mouse_x, last_rendered_mouse_y, 
                last_rendered_mouse_x, last_rendered_mouse_y, 10, 16, backbuffer);        

            /* now actually draw a cursor*/
            draw_icon(frontbuffer, xtc_context.mouse_x, xtc_context.mouse_y, cursor_icons[xtc_context.cursor_icon]);

            /* put the data to the screen */
            display_render(display, frontbuffer, last_rendered_mouse_x, last_rendered_mouse_y, 10, 16);
            display_render(display, frontbuffer, xtc_context.mouse_x, xtc_context.mouse_y, 10, 16);
        }
        
        struct window *win = xtc_context.focused_window;

        if (win && (IS_WINDOW_DRAGGED(win) || IS_WINDOW_RESIZED(win))) {
            int wf_width = win->absolute_width;
            int wf_height = win->absolute_height;

            if (IS_WINDOW_RESIZED(win)) {
                wf_width = win->resize_width;
                wf_height = win->resize_height;
            }
            
            /* draw a wireframe at the users's mouse coordinates (user is moving a window) */

            draw_wireframe(frontbuffer, win->x, win->y, wf_width, wf_height);
        
            if (win->x != last_rendered_wf_x || win->y != last_rendered_wf_y ||
                    (last_wf_height != wf_height || last_wf_width != wf_width)) {
                render_wireframe(display, backbuffer, last_rendered_wf_x,
                        last_rendered_wf_y, last_wf_width, last_wf_height);
                last_rendered_wf_x = win->x;
                last_rendered_wf_y = win->y;
                last_wf_height = wf_height;
                last_wf_width = wf_width;
            } 

            render_wireframe(display, frontbuffer, win->x, win->y, wf_width, wf_height);
        } 

        buf->dirty = 0;
        
        last_rendered_mouse_x = xtc_context.mouse_x;
        last_rendered_mouse_y = xtc_context.mouse_y;

        /* 
         * this is something that is in my libc but not an official function
         * it sleeps for a millisecond. 
         * We're not good enough for usleep() 
         */
        extern void msleep(int milliseconds);

        msleep(8);
    }

    canvas_destroy(frontbuffer);
    canvas_destroy(backbuffer);
    display_close(display);

    return NULL;
}

static void
init_session()
{
    pid_t child = fork();

    if (!child) {
        sleep(3);
        char *argv[] = {
            "sh",
            "/etc/xtc/xtcsession.rc",
            NULL
        };
        execv("/bin/sh", argv);
        exit(-1);
    }
}

static void
set_properties()
{
	char *background_col = getenv("BACKGROUND_COLOR");
    char *border_col = getenv("WINDOW_BORDER_COLOR");
    char *text_col = getenv("WINDOW_TEXT_COLOR");
    char *title_col = getenv("WINDOW_TITLE_COLOR");
    char *active_title_col = getenv("WINDOW_ACTIVE_TITLE_COLOR");
    char *active_text_col = getenv("WINDOW_ACTIVE_TEXT_COLOR");
    char *window_col = getenv("WINDOW_COLOR");
    char *chisel_dark_col = getenv("CHISEL_DARK_COLOR");
    char *chisel_light_col = getenv("CHISEL_LIGHT_COLOR");
	char *chisel_active_dark_col = getenv("CHISEL_ACTIVE_DARK_COLOR");
    char *chisel_active_light_col = getenv("CHISEL_ACTIVE_LIGHT_COLOR");

	if (background_col) {
		XTC_SET_PROPERTY(XTC_BACKGROUND_COL, strtol(background_col, NULL, 16));
		unsetenv("BACKGROUND_COLOR");
	}

    if (border_col) {
        XTC_SET_PROPERTY(XTC_WIN_BORDER_COL, strtol(border_col, NULL, 16));
        unsetenv("WINDOW_BORDER_COLOR");
    }
   
    if (text_col) {
        XTC_SET_PROPERTY(XTC_WIN_TEXT_COL, strtol(text_col, NULL, 16));
        unsetenv("WINDOW_TEXT_COLOR");
    }

    if (title_col) {
        XTC_SET_PROPERTY(XTC_WIN_TITLE_COL, strtol(title_col, NULL, 16));
        unsetenv("WINDOW_TITLE_COLOR");
    }

    if (active_title_col) {
        XTC_SET_PROPERTY(XTC_WIN_ACTIVE_TITLE_COL, strtol(active_title_col, NULL, 16));
        unsetenv("WINDOW_ACTIVE_TITLE_COLOR");
    }

    if (active_text_col) {
        XTC_SET_PROPERTY(XTC_WIN_ACTIVE_TEXT_COL, strtol(active_text_col, NULL, 16));
        unsetenv("WINDOW_ACTIVE_TEXT_COLOR");
    }

    if (window_col) {
        XTC_SET_PROPERTY(XTC_WIN_COL, strtol(window_col, NULL, 16));
        unsetenv("WINDOW_COLOR");
    }

	if (chisel_light_col) {
		XTC_SET_PROPERTY(XTC_CHISEL_LIGHT_COL, strtol(chisel_light_col, NULL, 16));
		unsetenv("CHISEL_LIGHT_COLOR");
	}
    if (chisel_dark_col) {
        XTC_SET_PROPERTY(XTC_CHISEL_DARK_COL, strtol(chisel_dark_col, NULL, 16));
        unsetenv("CHISEL_DARK_COLOR");
    }

    if (chisel_active_light_col) {
        XTC_SET_PROPERTY(XTC_CHISEL_ACTIVE_LIGHT_COL, strtol(chisel_active_light_col, NULL, 16));
        unsetenv("CHISEL_ACTIVE_LIGHT_COLOR");
    }
 
	if (chisel_active_dark_col) {
        XTC_SET_PROPERTY(XTC_CHISEL_ACTIVE_DARK_COL, strtol(chisel_active_dark_col, NULL, 16));
        unsetenv("CHISEL_ACTIVE_DARK_COLOR");
	}
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

	set_properties();

    memset(&xtc_context, 0, sizeof(xtc_context));

    thread_t input_thread;
    thread_t display_thread;
    thread_create(&input_thread, io_loop, NULL);
    thread_create(&display_thread, display_loop, NULL);

    init_session();

    server_listen();

    return 0;
}
