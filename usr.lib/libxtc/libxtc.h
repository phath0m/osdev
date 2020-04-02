#ifndef _LIBXTC_H
#define _LIBXTC_H

#include <libmemgfx/canvas.h>

#define XTC_EVT_CLICK       0x01
#define XTC_EVT_KEYPRESS    0x02
#define XTC_EVT_RESIZE      0x03

typedef int xtc_win_t;
typedef int xtc_client_t;

typedef struct {
    int     type;
    int     parameters[2];
} xtc_event_t;

int xtc_connect();
void xtc_disconnect();
xtc_win_t xtc_window_new(int width, int height, int flags);
int xtc_set_window_title(xtc_win_t win, const char *title);
int xtc_next_event(xtc_win_t win, xtc_event_t *event);
int xtc_clear(xtc_win_t win, int col);
int xtc_put_string(xtc_win_t win, int x, int y, const char *str, int col);
int xtc_redraw(xtc_win_t win);
int xtc_resize(xtc_win_t win, int width, int height);
canvas_t *xtc_open_canvas(xtc_win_t win, int flags);

#endif
