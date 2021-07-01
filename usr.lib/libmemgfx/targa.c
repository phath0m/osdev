#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "canvas.h"

struct targa_hdr {
    uint8_t     id_len;
    uint8_t     map_type;
    uint8_t     img_type;
    uint16_t    map_first;
    uint16_t    map_len;
    uint8_t     map_entry_size;
    uint16_t    x;
    uint16_t    y;
    uint16_t    width;
    uint16_t    height;
    uint8_t     bpp;
    uint8_t     misc;
} __attribute__((packed));

canvas_t *
canvas_from_targa(const char *path, int flags)
{
    int bpp;
    int image_size;
    int x;
    int y;

    color_t *pixels;
    pixbuf_t *pixbuf;
    uint8_t *image;
    uint8_t *rgb;
    FILE *fp;

    struct targa_hdr header;

    fp = fopen(path, "r");

    if (!fp) {
        return NULL;
    }

    fread(&header, 1, sizeof(header), fp);

    bpp = header.bpp / 8;
    image_size = bpp*header.width*header.height;
    image = calloc(1, image_size);
    
    fread(image, 1, image_size, fp);
    fclose(fp);

    pixbuf = pixbuf_new(header.width, header.height);
    pixels = pixbuf->pixels;
   
    for (y = 0; y < header.height; y++)
    for (x = 0; x < header.width; x++) {
        rgb = &image[(y*header.width+x)*bpp];

        pixels[y * header.width + x] = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
    }

    free(image);

    return canvas_from_mem(header.width, header.height, flags, pixbuf);
}
