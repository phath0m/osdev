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
    size_t bufsize;

    bufsize = width*height*sizeof(color_t);

    return calloc(1, bufsize+sizeof(pixbuf_t));
}

canvas_t * 
canvas_from_mem(int width, int height, int flags, pixbuf_t *pixels)
{
    canvas_t *canvas;

    canvas = calloc(1, sizeof(canvas_t));

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

void
canvas_resize(canvas_t *canvas, color_t col, int width, int height)
{
    color_t *oldpixels;
    color_t *newpixels;

    canvas->buffersize = width*height*sizeof(color_t);
    oldpixels = canvas->pixels->pixels;
    newpixels = calloc(1, canvas->buffersize);

    fast_memset_d(newpixels, col, width*height);

    for (int x = 0; x < MIN(width, canvas->width); x++) {
        for (int y = 0; y < MIN(height, canvas->height); y++) {
            newpixels[y * width + x] = oldpixels[y*canvas->width+x];
        }
    }

    memcpy(oldpixels, newpixels, canvas->buffersize);
    free(newpixels);

    canvas->width = width;
    canvas->height = height;

    canvas_invalidate_region(canvas, 0, 0, width, height);
}

canvas_t *
canvas_new(int width, int height, int flags)
{
    pixbuf_t *pixels;
    
    pixels = pixbuf_new(width, height);

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
    int a_y;
    int max_height;
    int max_width;
    int min_height;
    int min_width;
    int row_size;

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    max_width = width;
    max_height = height;
    min_width = canvas->width - x;
    min_height = canvas->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    row_size = width;

    for (a_y = 0; a_y < height; a_y++) {
        void *dst;
        
        dst = &canvas->pixels->pixels[canvas->width * (a_y + y) + x];

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
canvas_circle(canvas_t *canvas, int x, int y, int d, color_t col)
{
    int i;
    int j;
    int r;
    int x2, y2;    
    int pos;

    color_t *pixels;

    r = d / 2;
    pixels = canvas->pixels->pixels;

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+d, y+d);
    }

    for (i = 0; i <= d; i++) {
        for (j = 0; j <= d; j++) {
            x2 = i-r;
            y2 = j-r;

            if (x2*x2 + y2*y2 <= r*r+1) {
                pos = (canvas->width * (y + (r+y2)) + x + (r+x2));
                pixels[pos] = col;            
            }
        }
    }
}

void
canvas_line(canvas_t *canvas, int x0, int y0, int x1, int y1, color_t col)
{
    int dx;
    int dy;
    int err;
    int e2;
    int sx;
    int sy;
    int pos;

    color_t *pixels;

    pixels = canvas->pixels->pixels;

    dx = abs(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = abs(y1 - y0);
    sy = y0 < y1 ? 1 : -1; 
    err = (dx>dy ? dx : -dy) / 2;

    for (;;) {
        pos = (canvas->width * y0) + x0;

        pixels[pos] = col;

        if (x0 == x1 && y0 == y1) break;
        e2 = err;

        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }        
    }

}

void
canvas_rect(canvas_t *canvas, int x, int y, int width, int height, color_t col)
{
    bool right_face_visible;
    bool bottom_face_visible;


    int a_y;
    int a_x;
    int max_height;
    int max_width;
    int min_height;
    int min_width;

    color_t *pixels;

    max_width = width;
    max_height = height;
    min_width = canvas->width - x;
    min_height = canvas->height - y;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    right_face_visible = (width == max_width); /* is the right face visible? */
    bottom_face_visible = (height == max_height); /* is the buttom face visible? */

    pixels = canvas->pixels->pixels;

    for (a_y = 0; a_y < height; a_y++) {
        int pos;
        
        pos = canvas->width * (a_y + y) + x;

        pixels[pos] = col;
       
        if (right_face_visible) { 
            pixels[pos + width] = col;
        }
    }

    for (a_x = 0; a_x < width; a_x++) {
        int pos;
        
        pos = x + a_x;

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
    int a_y;
    int max_height;
    int max_width;
    int min_height;
    int min_width;

    color_t *dstpixels;
    color_t *srcpixels;

    max_width = width;
    max_height = height;
    min_width = canvas->width - x1;
    min_height = canvas->height - y1;

    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);

    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x1, y1, x1+width, y1+height);
    }

    dstpixels = canvas->pixels->pixels;
    srcpixels = subcanvas->pixels->pixels;
    
    for (a_y = 0; a_y < height; a_y++) {
        void *dst;
        void *src;

        dst = &dstpixels[canvas->width * (a_y + y1) + x1];
        src = &srcpixels[subcanvas->width * (a_y + y2) + x2];

        fast_memcpy_d(dst, src, width*4);
    }
}

void
canvas_putpixels(canvas_t *canvas, int x, int y, int width, int height, color_t *pixels)
{
    int a_x;
    int a_y;
    int max_width;
    int max_height;
    int min_width;
    int min_height;

    color_t *dstpixels;

    max_width = width;
    max_height = height;
    min_width = canvas->width - x;
    min_height = canvas->height - y;
    
    width = MIN(max_width, min_width);
    height = MIN(max_height, min_height);
    
    if (width <= 0 || height <= 0) {
        return;
    }

    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, x, y, x+width, y+height);
    }

    dstpixels = canvas->pixels->pixels;

    for (a_x = 0; a_x < width; a_x++)
    for (a_y = 0; a_y < height; a_y++) {
        int bitmap_pos;
        int buf_pos;

        buf_pos = (canvas->width * (a_y + y)) + (a_x + x);
        bitmap_pos = height * a_y + a_x;

        if (pixels[bitmap_pos] != 0xFFFFFFFF) {
            dstpixels[buf_pos] = pixels[bitmap_pos];
        }
    }
}

void
canvas_putc(canvas_t *canvas, int x, int y, int ch, color_t col)
{
    static uint32_t pixel_data[FONT_WIDTH*FONT_HEIGHT];

    int i;
    int j;
    uint8_t *c;

    c = number_font[ch];

    for (i = 0; i < FONT_WIDTH; i++)
    for (j = 0; j < FONT_HEIGHT; j++) {
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
    int i;

    for (i = 0; i < strlen(str); i++) {
        canvas_putc(canvas, x + (i * (FONT_WIDTH + 2)), y, str[i], col);
    }
}

void
canvas_scroll(canvas_t *canvas, int amount, color_t fill)
{
    int start_index;
    int copysize;

    color_t *dstpixels;
    color_t *srcpixels;

    start_index = amount * canvas->width;
    copysize = (canvas->height - amount) * canvas->width;
    
    dstpixels = canvas->pixels->pixels;
    srcpixels = &canvas->pixels->pixels[start_index];

    fast_memcpy_d(dstpixels, srcpixels, copysize*sizeof(color_t));
    
    if (canvas->enable_partial_render) {
        update_dirty_region(canvas->pixels, 0, 0, canvas->width, canvas->height);
    }
}

void
canvas_scale(canvas_t *canvas, int width, int height)
{
    int dst_x;
    int dst_y;
    int src_x;
    int src_y;
    int x_ratio;
    int y_ratio;
    color_t *oldpixels;
    color_t *newpixels;
    pixbuf_t *newbuf;

    newbuf = pixbuf_new(width, height); 

    oldpixels = canvas->pixels->pixels;
    newpixels = newbuf->pixels;

    x_ratio = (canvas->width << 16) / width + 1;
    y_ratio = (canvas->height << 16) / height + 1;

    for (dst_y = 0; dst_y < height; dst_y++)
    for (dst_x = 0; dst_x < width; dst_x++) {
        src_x = ((dst_x * x_ratio) >> 16);
        src_y = ((dst_y * y_ratio) >> 16);

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
