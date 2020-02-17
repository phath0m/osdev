#ifndef _LIBMEMGFX_CANVAS_H
#define _LIBMEMGFX_CANVAS_H

#include <stdint.h>

typedef uint32_t color_t;

typedef struct canvas {
    int         width;
    int         height;
    int         buffersize;
    color_t *   pixels; /* points to the buffer (front buffer, or, backbuffer)*/
    color_t *   backbuffer; /* backbuffer, only used if double-buffered */
    color_t *   frontbuffer; /* pointer to the buffer that is currently visible */
} canvas_t;

#define CANVAS_DOUBLE_BUFFER        0x01
#define CANVAS_ALPHA_BLEND          0x02

canvas_t *canvas_new(int width, int height, int flags);
canvas_t *canvas_from_mem(int width, int height, int flags, color_t *pixels);

void canvas_destroy(canvas_t *canvas);
void canvas_clear(canvas_t *canvas, color_t col);
void canvas_fill(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_rect(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_paint(canvas_t *canvas);
void canvas_putc(canvas_t *canvas, int x, int y, int ch, color_t col);
void canvas_puts(canvas_t *canvas, int x, int y, const char *str, color_t col);
void canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *colors);
void canvas_putcanvas(canvas_t *canvas, int x, int y, canvas_t *other);
void canvas_scroll(canvas_t *canvas, int amount, color_t fill);
#endif
