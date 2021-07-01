#ifndef _LIBMEMGFX_CANVAS_H
#define _LIBMEMGFX_CANVAS_H

#include <stdint.h>

typedef uint32_t color_t;

/* used to track the modified region if CANVAS_PARTIAL_RENDER is set */
typedef struct pixbuf {
    int         dirty;
    int         min_x;
    int         min_y;
    int         max_x;
    int         max_y;
    color_t     pixels[];
} pixbuf_t;

/* actual canvas, represents a linear array of pixels in memory */
typedef struct canvas {
    int             width;
    int             height;
    int             buffersize;
    int             enable_partial_render; /* set if CANVAS_PARTIAL_RENDER is enabled */
    pixbuf_t *      pixels; /* frontbuffer, or, backbuffer if double buffer is enabled */
    pixbuf_t *      frontbuffer; /* actual buffer */
    pixbuf_t *      backbuffer; /* only used if double-buffering is enabled */
} canvas_t;

#define CANVAS_DOUBLE_BUFFER        0x01 /* Should output be double buffered */
#define CANVAS_ALPHA_BLEND          0x02 /* NOT IMPLEMENTED YET */
#define CANVAS_PARTIAL_RENDER       0x04 /* Optimization: If set, only the smallest rectangle containing */
                                         /* modified pixels will actually be written rendered */

pixbuf_t *pixbuf_new(int width, int height);

canvas_t *canvas_new(int width, int height, int flags);
canvas_t *canvas_from_mem(int width, int height, int flags, pixbuf_t *pixels);
canvas_t *canvas_from_targa(const char *path, int flags);

void canvas_destroy(canvas_t *canvas);
void canvas_circle(canvas_t *, int, int, int, color_t);
void canvas_clear(canvas_t *canvas, color_t col);
void canvas_fill(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_line(canvas_t *, int, int, int, int, color_t);
void canvas_rect(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_resize(canvas_t *canvas, color_t col, int width, int height);
void canvas_paint(canvas_t *canvas);
void canvas_invalidate_region(canvas_t *canvas, int x, int y, int width, int height);
void canvas_putc(canvas_t *canvas, int x, int y, int ch, color_t col);
void canvas_puts(canvas_t *canvas, int x, int y, const char *str, color_t col);
void canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *colors);

void canvas_putcanvas(canvas_t *canvas, int x1, int y1,
    int x2, int y2, int width, int height, canvas_t *subcanvas);

void canvas_scroll(canvas_t *canvas, int amount, color_t fill);
void canvas_scale(canvas_t *canvas, int width, int height);

#endif
