#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxtc.h>
#include <unistd.h>
#include <sys/time.h>
#include <libmemgfx/canvas.h>


/*
 * newlib doesn't have a cos/sin implementation by default so I just snagged code from here:
 * https://gist.github.com/giangnguyen2412/bcab883b5a53b437b980d7be9745beaf
 */

#define M_PI_M_2 M_PI*2

int
compare_float(double f1, double f2)
{
    double precision;
    
    precision = 0.00000000000000000001;

    if ((f1 - precision) < f2)
        return -1;
    else if ((f1 + precision) > f2)
        return 1;
    else
        return 0;
}

double
cos(double x)
{
    if (x < 0.0f) x = -x;

    if (0 <= compare_float(x,M_PI_M_2)) {
        do {
            x -= M_PI_M_2;
        } while(0 <= compare_float(x,M_PI_M_2));
    }

    if ((0 <= compare_float(x, M_PI)) && (-1 == compare_float(x, M_PI_M_2))) {
        x -= M_PI;
        return ((-1)*(1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * ( 1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f )*(1.0f - (x*x/90.0f)*(1.0f - (x*x/132.0f)*(1.0f - (x*x/182.0f)))))))));
    }
    return 1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * ( 1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f )*(1.0f - (x*x/90.0f)*(1.0f - (x*x/132.0f)*(1.0f - (x*x/182.0f)))))));
}

double
sin(double x)
{
    return cos(x-M_PI_2);
}

void
draw_line_from_center(canvas_t *canvas, int cX, int cY, int radius, int angle, color_t col)
{
    double x;
    double y;
    double radian;

    radian = ((angle + 270) % 360) * 0.0174532925;
    y = radius + cX + (radius * sin(radian));
    x = radius + cY + (radius * cos(radian));

    canvas_line(canvas, radius + cX, radius + cY, (int)x, (int)y, col);
}

void
draw_ticks(canvas_t *canvas, int cX, int cY, int radius)
{
    int i;

    for (i = 0; i < 60; i++) {
        draw_line_from_center(canvas, cX, cY, radius, i * 6, 0x0);
    }

    canvas_circle(canvas, cX+8, cY+8, (radius-8)*2, 0xFFFFFF);

    for (i = 0; i < 12; i++) {
        draw_line_from_center(canvas, cX, cY, radius, i * 30, 0x0);
    }

    canvas_circle(canvas, cX+13, cY+13, (radius-13)*2, 0xFFFFFF);
}

void
draw_main(canvas_t *canvas)
{
    canvas_clear(canvas, 0xFFFFFF);
    draw_ticks(canvas, 2, 2, 100);
}

int
main(int argc, const char *argv[])
{
    int angle_hour;
    int angle_min;
    int angle_sec;
    int prev_angle_hour;
    int prev_angle_min;
    int prev_angle_sec;
    time_t now;
    xtc_event_t event;
    xtc_win_t win;

    canvas_t *canvas;

    struct tm tm;

    if (xtc_connect() != 0) {
        fprintf(stderr, "could not connect to server!\n");
        return -1;
    }

    prev_angle_hour = 0;
    prev_angle_min = 0;
    prev_angle_sec = 0;

    win = xtc_window_new(208, 208, 0);
    canvas = xtc_open_canvas(win, 0);

    draw_main(canvas);

    xtc_set_window_title(win, "Clock");

    for (;;) {
        now = time(NULL);
        localtime_r(&now, &tm);

        angle_min = tm.tm_min * 6;
        angle_hour = (tm.tm_hour % 12) * 30;
        angle_sec = tm.tm_sec * 6;

        draw_line_from_center(canvas, 32, 32, 70, prev_angle_hour, 0xFFFFFF);
        draw_line_from_center(canvas, 22, 22, 80, prev_angle_min, 0xFFFFFF);
        draw_line_from_center(canvas, 22, 22, 80, prev_angle_sec, 0xFFFFFF);

        draw_line_from_center(canvas, 32, 32, 70, angle_hour, 0x0);
        draw_line_from_center(canvas, 22, 22, 80, angle_min, 0x0);
        draw_line_from_center(canvas, 22, 22, 80, angle_sec, 0xFF0000);

        canvas_invalidate_region(canvas, 0, 0, 204, 204);

        prev_angle_min = angle_min;
        prev_angle_hour = angle_hour;
        prev_angle_sec = angle_sec;

        sleep(1);

        while (xtc_poll_events(win) > 0) {
            xtc_next_event(win, &event);
            switch (event.type) {
              case XTC_EVT_RESIZE:
                canvas->width = event.parameters[0];
                canvas->height = event.parameters[1];
                xtc_resize(win, 0xFFFFFF, event.parameters[0], event.parameters[1]);
                draw_main(canvas);
              default:
                  break;
             }
        }
    }

    return 0;
}
