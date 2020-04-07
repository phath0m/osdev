#ifndef _WM_H
#define _WM_H

#include <collections/list.h>
#include "window.h"


#define CURSOR_ICON_NORMAL      0x00
#define CURSOR_ICON_RESIZE      0x01

/* window manager context */
struct wmctx {
    int             mouse_x; /* actual mouse X coordinates */
    int             mouse_y; /* actual mouse Y coordinates */
    int             mouse_delta_x; /* mouse X movement */
    int             mouse_delta_y; /* mouse Y movement */
    int             redraw_flag; /* if set; will force all windows to be re-drawn */
    int             cursor_icon; /* */
    struct list     windows; /* list of all visible windows */
    struct window * focused_window; /* currently focused window */
};

void wm_add_window(struct window *win);
void wm_remove_window(struct window *win);
void wm_redraw();

#endif
