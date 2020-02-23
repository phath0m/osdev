#ifndef _WM_H
#define _WM_H

#include <collections/list.h>
#include "window.h"

/* window manager context */
struct wmctx {
    int             mouse_x; /* actual mouse X coordinates */
    int             mouse_y; /* actual mouse Y coordinates */
    int             mouse_delta_x; /* mouse X movement */
    int             mouse_delta_y; /* mouse Y movement */
    int             redraw_flag; /* if set; will force all windows to be re-drawn */
    struct list     windows; /* list of all visible windows */
    struct window * focused_window; /* currently focused window */
};

void wm_add_window(struct wmctx *ctx, struct window *win);
void wm_remove_window(struct wmctx *ctx, struct window *win);

#endif
