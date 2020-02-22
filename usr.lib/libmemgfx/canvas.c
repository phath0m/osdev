#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "canvas.h"
#include "font.h"
#include "utils.h"

/* updates the "dirty" region of the pixbuf is partial rendering is enabled */
static inline void
update_dirty_region(pixbuf_t *region, int x1, int y1, int x2, int y2)
{
    if (!region->dirty) {
        region->min_x = x1;
        region->min_y = y1;
        region->max_x = x2;
        region->max_y = y2;
        region->dirty = 1;
        return;
    }

    if (x1 < region->min_x) {
        region->min_x = x1;
    }

    if (y1 < region->min_y) {
        region->min_y = y1;
    }

    if (x2 > region->max_x) {
        region->max_x = x2;
    }

    if (y2 > region->max_y) {
        region->max_y = y2;
    }
}

pixbuf_t *
pixbuf_new(int width, int height)
{
    int bufsize = width*height*sizeof(color_t);
    return calloc(1, bufsize+sizeof(pixbuf_t));
}

canvas_t * 
canvas_from_mem(int width, int height, int flags, pixbuf_t *pixels)
{
    canvas_t *canvas = calloc(1, sizeof(canvas_t));

    canvas->buffersize = width*height*sizeof(color_t);
    canvas->frontbuffer = pixels;

    if ((flags & CANVAS_DOUBLE_BUFFER)) {
        canvas->backbuffer = pixbuf_new(width, height);
        canvas->pixels = canvas->backbuffer;
    } else {
        canvas->pixels = canvas->frontbuffer;
    }

    if ((flags & CANVAS_PARTIAL_RENDER)) {
        canvas->enable_partial_render = 1;
        update_dirty_region(canvas->pixels, 0, 0, width, height);
    }

    canvas->width = width;
    canvas->height = height;

    return canvas;
}

canvas_t *
canvas_new(int width, int height, int flags)
{
    pixbuf_t *pixels = pixbuf_new(width, height);

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
    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    int max_width = width;
    int max_height = height;
    int min_width = canvas->width - x;
    int min_height = canvas->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    int row_size = width;

    for (int a_y = 0; a_y < height; a_y++) {
        void *dst = &canvas->pixels->pixels[canvas->width * (a_y + y) + x];

        fast_memset_d(dst, col, row_size);
    }
}

void
canvas_invalidate_region(canvas_t *canvas, int x, int y, int width, int height)
{
    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }
}

void
canvas_rect(canvas_t *canvas, int x, int y, int width, int height, color_t col)
{
    int max_width = width;
    int max_height = height;
    int min_width = canvas->width - x;
    int min_height = canvas->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    bool right_face_visible = (width == max_width); /* is the right face visible? */
    bool bottom_face_visible = (height == max_height); /* is the buttom face visible? */

    color_t *pixels = canvas->pixels->pixels;

    for (int a_y = 0; a_y < height; a_y++) {
        int pos = canvas->width * (a_y + y) + x;
        pixels[pos] = col;
       
        if (right_face_visible) { 
            pixels[pos + width] = col;
        }
    }

    for (int a_x = 0; a_x < width; a_x++) {
        int pos = x + a_x;
        pixels[canvas->width * y + pos] = col;
        
        if (bottom_face_visible) {
            pixels[canvas->width * (y + height) + pos] = col;
        }
    }
}

void
canvas_paint(canvas_t *canvas)
{
    if (!canvas->frontbuffer || !canvas->backbuffer) {
        return;
    }

    memcpy(canvas->frontbuffer, canvas->backbuffer, sizeof(pixbuf_t) + canvas->buffersize);
}

void
canvas_putcanvas(canvas_t *canvas, int x1, int y1, int x2, int y2, int width, int height, canvas_t *subcanvas)
{
    int max_width = width;
    int max_height = height;
    int min_width = canvas->width - x1;
    int min_height = canvas->height - y1;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x1, y1, x1+width, y1+height);
    }

    color_t *dstpixels = canvas->pixels->pixels;
    color_t *srcpixels = subcanvas->pixels->pixels;
    
    for (int a_y = 0; a_y < height; a_y++) {
        void *dst = &dstpixels[canvas->width * (a_y + y1) + x1];
        void *src = &srcpixels[subcanvas->width * (a_y + y2) + x2];

        fast_memcpy_d(dst, src, width*4);
    }
}

void
canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *pixels)
{
    int max_width = width;
    int max_height = height;
    int min_width = canvas->width - x;
    int min_height = canvas->height - y;
    
    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);
    
    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    color_t *dstpixels = canvas->pixels->pixels;

    for (int a_x = 0; a_x < width; a_x++)
    for (int a_y = 0; a_y < height; a_y++) {
        int buf_pos = (canvas->width * (a_y + y)) + (a_x + x);
        int bitmap_pos = height * a_y + a_x;
        if (pixels[bitmap_pos] != 0xFFFFFFFF) {
            dstpixels[buf_pos] = pixels[bitmap_pos];
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
    
    color_t *dstpixels = canvas->pixels->pixels;
    color_t *srcpixels = &canvas->pixels->pixels[start_index];

    fast_memcpy_d(dstpixels, srcpixels, copysize*sizeof(color_t));
    
    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, 0, 0, canvas->width, canvas->height);
    }
}

void
canvas_scale(canvas_t *canvas, int width, int height)
{
    pixbuf_t *newbuf = pixbuf_new(width, height); 

    color_t *oldpixels = canvas->pixels->pixels;
    color_t *newpixels = newbuf->pixels;

    int x_ratio = (canvas->width << 16) / width + 1;
    int y_ratio = (canvas->height << 16) / height + 1;

    for (int dst_y = 0; dst_y < height; dst_y++)
    for (int dst_x = 0; dst_x < width; dst_x++) {
        int src_x = ((dst_x * x_ratio) >> 16);
        int src_y = ((dst_y * y_ratio) >> 16);

        newpixels[dst_y*width+dst_x] = oldpixels[src_y*canvas->width+src_x];
    }

    free(canvas->pixels);

    canvas->pixels = newbuf;
    canvas->width = width;
    canvas->height = height;
    canvas->buffersize = width*height*sizeof(color_t);
    
    if (canvas->backbuffer) {
        canvas->backbuffer = newbuf;
        free(canvas->frontbuffer);
        canvas->frontbuffer = pixbuf_new(width, height);
    } else {
        canvas->frontbuffer = newbuf;
    }
}
