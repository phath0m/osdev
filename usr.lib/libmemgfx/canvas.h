#ifndef _LIBMEMGFX_CANVAS_H
#define _LIBMEMGFX_CANVAS_H

#include <stdint.h>

typedef uint32_t color_t;

typedef struct canvas {
    int         width;
    int         height;
    color_t *   pixels;
} canvas_t;

canvas_t *canvas_new(int width, int height);

void canvas_destroy(canvas_t *canvas);
void canvas_clear(canvas_t *canvas, color_t col);
void canvas_fill(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_rect(canvas_t *canvas, int x, int y, int width, int height, uint32_t col);
void canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *colors);
void canvas_putcanvas(canvas_t *canvas, int x, int y, canvas_t *other);

#endif
