#include <stdlib.h>
#include <string.h>
#include "canvas.h"
#include "font.h"


#define MIN(a,b) (((a)<(b))?(a):(b))


static inline void 
fast_memcpy_d(void *dst, const void *src, size_t nbyte)
{
    asm volatile("cld\n\t"
                 "rep ; movsd"
                 : "=D" (dst), "=S" (src)
                 : "c" (nbyte / 4), "D" (dst), "S" (src)
                 : "memory");
}

static inline void
fast_memset_d(void *dst, uint32_t val, size_t nbyte)
{
    asm volatile("cld; rep stosl\n\t"
            : "+c"(nbyte), "+D" (dst) : "a" (val)
            : "memory");
}

canvas_t * 
canvas_from_mem(int width, int height, int flags, color_t *pixels)
{
    canvas_t *canvas = calloc(1, sizeof(canvas_t));

    canvas->buffersize = width*height*sizeof(color_t);
    canvas->frontbuffer = pixels;

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

canvas_t *
canvas_new(int width, int height, int flags)
{
    color_t *pixels = calloc(1, width*height*4);
    
    return canvas_from_mem(width, height, flags, pixels);
}

void
canvas_clear(canvas_t *canvas, color_t col)
{
    canvas_fill(canvas, 0, 0, canvas->width, canvas->height, col);
}

void
canvas_fill(canvas_t *canvas, int x, int y, int width, int height, color_t col)
{
    int row_size = width;
    for (int a_y = 0; a_y < height; a_y++) {
        void *dst = &canvas->pixels[canvas->width * (a_y + y) + x];

        fast_memset_d(dst, col, row_size);
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

    for (int a_y = 0; a_y < height; a_y++) {
        void *dst = &canvas->pixels[canvas->width * (a_y + y) + x];
        void *src = &pixels[subcanvas->width * a_y];

        fast_memcpy_d(dst, src, width*4);
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

void
canvas_scroll(canvas_t *canvas, int amount, color_t fill)
{
    int start_index = amount * canvas->width;
    int copysize = (canvas->height - amount) * canvas->width;
    
    fast_memcpy_d(canvas->pixels, &canvas->pixels[start_index], copysize*sizeof(color_t));
}

void
canvas_scale(canvas_t *canvas, int width, int height)
{
    color_t *newpixels = calloc(1, width*height*sizeof(color_t));
    color_t *oldpixels = canvas->pixels;

    int x_ratio = (canvas->width << 16) / width + 1;
    int y_ratio = (canvas->height << 16) / height + 1;

    for (int dst_y = 0; dst_y < height; dst_y++)
    for (int dst_x = 0; dst_x < width; dst_x++) {
        int src_x = ((dst_x * x_ratio) >> 16);
        int src_y = ((dst_y * y_ratio) >> 16);

        newpixels[dst_y*width+dst_x] = oldpixels[src_y*canvas->width+src_x];
    }

    canvas->pixels = newpixels;
    canvas->width = width;
    canvas->height = height;

    free(oldpixels);   
    
    if (canvas->backbuffer) {
        canvas->backbuffer = newpixels;
        free(canvas->frontbuffer);
        canvas->frontbuffer = calloc(1, width*height*sizeof(color_t));
    } else {
        canvas->frontbuffer = newpixels;
    }
}
