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

static void set_redraw(struct wmctx *ctx);
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

void
wm_add_window(struct wmctx *ctx, struct window *win)
{
    list_append(&ctx->windows, win);
}

void
wm_remove_window(struct wmctx *ctx, struct window *win)
{
    list_remove(&ctx->windows, win);

    set_redraw(ctx);
}

/* mark a window as being active and trigger a re-draw */
static void
set_active_window(struct wmctx *ctx, struct window *win)
{
    if (ctx->focused_window == win) {
        return;
    }

    if (ctx->focused_window) {
        ctx->focused_window->active = 0;
        ctx->focused_window->redraw = 1;
    }

    ctx->focused_window = win;
    win->active = 1;
    win->redraw = 1;
}

/* force re-draw of everything */
static void
set_redraw(struct wmctx *ctx)
{
    ctx->redraw_flag = 1;

    list_iter_t iter;

    list_get_iter(&ctx->windows, &iter);

    struct window *win = NULL;

    while (iter_move_next(&iter, (void**)&win)) {
        win->redraw = 1;
    }
}

/* dispatch actual mouse events to the window */
static void
handle_click(struct wmctx *ctx, struct click_event *event)
{
    if (ctx->focused_window && ctx->focused_window->dragged) {
        ctx->focused_window->x = event->x;
        ctx->focused_window->y = event->y;
        return;
    }

    list_iter_t iter;

    list_get_iter(&ctx->windows, &iter);

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
            list_remove(&ctx->windows, active_win);
            list_append(&ctx->windows, active_win);
        }
      
        set_active_window(ctx, active_win); 
    }
}

/* handle hover events */
static void
handle_hover(struct wmctx *ctx, struct click_event *event)
{
    list_iter_t iter;

    list_get_iter(&ctx->windows, &iter);

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

    if (active_win->dragged) {
        active_win->dragged = 0;
        active_win->redraw = 1;
        set_redraw(ctx);
    }

    window_hover(active_win, event);
}

/* handle key event */
static void
handle_key_event(struct wmctx *ctx, struct kbd_event *eventbuf)
{
    if (ctx->focused_window) {
        window_keypress(ctx->focused_window, eventbuf->scancode);
    }
}

/* actually figure out wtf the mouse is doing */
static void
handle_mouse_event(struct wmctx *ctx, struct mouse_event *eventbuf)
{
    if (eventbuf->x == 1 || eventbuf->x == -1) {
        eventbuf->x = 0;
    }

    if (eventbuf->y == 1 || eventbuf->y == -1) {
        eventbuf->y = 0;
    }
    /*
    actual_mouse_delta_x = eventbuf->x;
    actual_mouse_delta_y = eventbuf->y;

    switch (eventbuf->event) {
        case MOUSE_LEFT_CLICK:
        case MOUSE_RIGHT_CLICK:
        case MOUSE_MOVE:
            
            ctx->mouse_delta_x = eventbuf->x;
            ctx->mouse_delta_y = eventbuf->y;
            break;
        case MOUSE_NOP:
            ctx->mouse_delta_x = eventbuf->x;
            ctx->mouse_delta_y = eventbuf->y;
            return;
        default:
            break;
    }*/

    if (eventbuf->x != 0 && eventbuf->x != actual_mouse_delta_x) {
        ctx->mouse_delta_x = eventbuf->x;
        actual_mouse_delta_x = eventbuf->x;
    }

    if (eventbuf->y != 0 && eventbuf->y != actual_mouse_delta_x) {
        ctx->mouse_delta_y = eventbuf->y;
        actual_mouse_delta_y = eventbuf->y;
    }

    struct click_event event;
    event.x = ctx->mouse_x;
    event.y = ctx->mouse_y;

    switch (eventbuf->event) {
        case MOUSE_MOVE:
            handle_hover(ctx, &event);
            break;
        case MOUSE_LEFT_CLICK:
            handle_click(ctx, &event);
            break;
        default:
            break;
    }
}

/* thread responsible for reading from both the keyboard and the mouse */
static void *
io_loop(void *argp)
{
    struct wmctx *ctx = (struct wmctx*)argp;
    inmux_t *inmux = inmux_open();

    for (;;) {
        struct inevent event;

        inmux_next_event(inmux, &event);

        switch (event.type) {
            case INMUX_KBD:
                handle_key_event(ctx, &event.un.kbd_event);
            case INMUX_MOUSE:
                handle_mouse_event(ctx, &event.un.mouse_event);
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
draw_windows(struct wmctx *ctx, canvas_t *canvas)
{
    list_iter_t iter;

    list_get_iter(&ctx->windows, &iter);

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

/* thread responsible for rendering the screen, roughly 120 times a second */
static void *
display_loop(void *argp)
{
    struct wmctx *ctx = (struct wmctx*)argp;
    display_t *display = display_open();

    int width = display_width(display);
    int height = display_height(display);

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

    ctx->redraw_flag = 1;

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

    int mouse_bounds_x = width - 8;
    int mouse_bounds_y = height - 12;

    for (;;) {
        if (ctx->redraw_flag) {
            canvas_putcanvas(backbuffer, 0, 0, 0, 0, bg->width, bg->height, bg);
            ctx->redraw_flag = 0;
        }

        ctx->mouse_x += ctx->mouse_delta_x;
        ctx->mouse_y -= ctx->mouse_delta_y;
        
        ctx->mouse_delta_x = 0;
        ctx->mouse_delta_y = 0;

        if (ctx->mouse_x > mouse_bounds_x) {
            ctx->mouse_x = mouse_bounds_x;
        }

        if (ctx->mouse_y > mouse_bounds_y) {
            ctx->mouse_y = mouse_bounds_y;
        }

        if (ctx->mouse_x < 0) {
            ctx->mouse_x = 0;
        }

        if (ctx->mouse_y < 0) {
            ctx-> mouse_y = 0;
        }

        draw_windows(ctx, backbuffer);

        pixbuf_t *buf = backbuffer->pixels;
        
        int dirty_width = buf->max_x - buf->min_x;
        int dirty_height = buf->max_y - buf->min_y;

        canvas_putcanvas(frontbuffer, buf->min_x, buf->min_y, buf->min_x,
            buf->min_y, dirty_width, dirty_height, backbuffer);

        if (buf->dirty) { 
            display_render(display, frontbuffer, buf->min_x, buf->min_y, dirty_width, dirty_height);
        }

        if (last_rendered_mouse_x != ctx->mouse_x || last_rendered_mouse_y != ctx->mouse_y) {
            /* re-draw the cursor at the current mouse position */

            /* first; re-draw over the previous location */
            canvas_putcanvas(frontbuffer, last_rendered_mouse_x, last_rendered_mouse_y, 
                last_rendered_mouse_x, last_rendered_mouse_y, 8, 12, backbuffer);        

            /* now actually draw a cursor*/
            canvas_putc(frontbuffer, ctx->mouse_x, ctx->mouse_y, 'x', 0x0);
     
            /* put the data to the screen */
            display_render(display, frontbuffer, last_rendered_mouse_x, last_rendered_mouse_y, 8, 12);
            display_render(display, frontbuffer, ctx->mouse_x, ctx->mouse_y, 8, 12);
        }
        
        if (ctx->focused_window && ctx->focused_window->dragged) {
           
            /* draw a wireframe at the users's mouse coordinates (user is moving a window) */

            struct window *win = ctx->focused_window;
            draw_wireframe(frontbuffer, win->x, win->y, win->width, win->height);
        
            if (win->x != last_rendered_wf_x || win->y != last_rendered_wf_y) {
                render_wireframe(display, backbuffer, last_rendered_wf_x,
                        last_rendered_wf_y, win->width, win->height);
                last_rendered_wf_x = win->x;
                last_rendered_wf_y = win->y;
            } 

            render_wireframe(display, frontbuffer, win->x, win->y, win->width, win->height);
        } 

        buf->dirty = 0;
        
        last_rendered_mouse_x = ctx->mouse_x;
        last_rendered_mouse_y = ctx->mouse_y;

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

    static struct wmctx ctx;

    memset(&ctx, 0, sizeof(ctx));

    thread_t input_thread;
    thread_t display_thread;
    thread_create(&input_thread, io_loop, &ctx);
    thread_create(&display_thread, display_loop, &ctx);

    init_session();

    server_listen(&ctx);

    return 0;
}
