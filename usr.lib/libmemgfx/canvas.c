#include <stdlib.h>
#include <string.h>
#include "canvas.h"
#include "font.h"

canvas_t *
canvas_new(int width, int height)
{
    canvas_t *canvas = calloc(1, sizeof(canvas_t));
    canvas->pixels = calloc(1, width*height*sizeof(color_t));
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

