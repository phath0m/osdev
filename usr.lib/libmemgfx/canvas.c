#include <stdlib.h>
#include <string.h>
#include "canvas.h"
#include "font.h"


#define MIN(a,b) (((a)<(b))?(a):(b))

canvas_t *
canvas_new(int width, int height, int flags)
{
    canvas_t *canvas = calloc(1, sizeof(canvas_t));
    canvas->buffersize = width*height*sizeof(color_t);

    canvas->frontbuffer = calloc(1, canvas->buffersize);
    
    if ((flags & CANVAS_DOUBLE_BUFFER)) {
        canvas->backbuffer = calloc(1, canvas->buffersize);
        canvas->pixels = canvas->backbuffer;
    } else {
        canvas->pixels = canvas->frontbuffer;
    }

    canvas->width = width;
    canvas->height = height;

    return canvas;
}

void
canvas_clear(canvas_t *canvas, color_t col)
{
    canvas_fill(canvas, 0, 0, canvas->width, canvas->height, col);
}

void
canvas_fill(canvas_t *canvas, int x, int y, int width, int height, color_t col)
{
    for (int a_x = 0; a_x < width; a_x++)
    for (int a_y = 0; a_y < height; a_y++) {
        int pos = canvas->width * (a_y + y) + (a_x + x);
        canvas->pixels[pos] = col;
    }
}

void
canvas_rect(canvas_t *canvas, int x, int y, int width, int height, color_t col)
{
    for (int a_y = 0; a_y < height; a_y++) {
        int pos = canvas->width * (a_y + y) + x;
        canvas->pixels[pos] = col;
        canvas->pixels[pos + width] = col;
    }

    for (int a_x = 0; a_x < width; a_x++) {
        int pos = x + a_x;
        canvas->pixels[canvas->width * y + pos] = col;
        canvas->pixels[canvas->width * (y + height) + pos] = col;
    }
}

void
canvas_paint(canvas_t *canvas)
{
    if (!canvas->frontbuffer || !canvas->backbuffer) {
        return;
    }

    memcpy(canvas->frontbuffer, canvas->backbuffer, canvas->buffersize);
}

void
canvas_putcanvas(canvas_t *canvas, int x, int y, canvas_t *subcanvas)
{
    int max_width = subcanvas->width;
    int max_height = subcanvas->height;
    int min_width = canvas->width - x + max_width;
    int min_height = canvas->height - y;

    int width = MIN(max_width, min_width);
    int height = MIN(max_height, min_height);

    color_t *pixels = subcanvas->frontbuffer;

    for (int a_x = 0; a_x < width; a_x++)
    for (int a_y = 0; a_y < height; a_y++) {
        int canvas_pos = (canvas->width * (a_y + y)) + (a_x + x);
        int subcanvas_pos = subcanvas->width * a_y + a_x;

        if (pixels[subcanvas_pos] != 0xFFFFFFFF) {
            canvas->pixels[canvas_pos] = pixels[subcanvas_pos];
        }
    }
}

void
canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *pixels)
{
    for (int a_x = 0; a_x < width; a_x++)
    for (int a_y = 0; a_y < height; a_y++) {
        int buf_pos = (canvas->width * (a_y + y)) + (a_x + x);
        int bitmap_pos = height * a_y + a_x;
        if (pixels[bitmap_pos] != 0xFFFFFFFF) {
            canvas->pixels[buf_pos] = pixels[bitmap_pos];
        }
    }
}

void
canvas_putc(canvas_t *canvas, int x, int y, int ch, color_t col)
{
    static uint32_t pixel_data[FONT_WIDTH*FONT_HEIGHT];

    uint8_t *c = number_font[ch];

    for (int i = 0; i < FONT_WIDTH; i++)
    for (int j = 0; j < FONT_HEIGHT; j++) {
        if (c[j] & (1 << (8-i))) {
            pixel_data[j*FONT_HEIGHT+i] = col;
        } else {
            pixel_data[j*FONT_HEIGHT+i] = 0xFFFFFFFF;
        }
    }

    canvas_putpixels(canvas, x, y, FONT_WIDTH, FONT_HEIGHT, pixel_data);
}

void
canvas_puts(canvas_t *canvas, int x, int y, const char *str, color_t col)
{
    for (int i = 0; i < strlen(str); i++) {
        canvas_putc(canvas, x + (i * (FONT_WIDTH + 2)), y, str[i], col);
    }
}

