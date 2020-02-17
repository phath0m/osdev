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
    FILE *fp = fopen(path, "r");

    if (!fp) {
        return NULL;
    }

    struct targa_hdr header;

    fread(&header, 1, sizeof(header), fp);

    int bpp = header.bpp / 8;

    color_t *pixels = calloc(1, header.width*header.height*sizeof(uint32_t));
   
    //for (int y = header.height - 1; y >= 0; y--)
    for (int y = 0; y < header.height; y++)
    for (int x = 0; x < header.width; x++) {
        uint8_t rgb[4];

        fread(rgb, 1, bpp, fp);

        pixels[y * header.width + x] = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
    }

    fclose(fp);
 
    return canvas_from_mem(header.width, header.height, flags, pixels);
}
