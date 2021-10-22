#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "canvas.h"
#include "sfn.h"

#define SSFN_IMPLEMENTATION                         /* use the normal renderer implementation */
#include "ssfn_impl.h"

struct sfn_font {
    ssfn_t  ssfn_ctx;
};

int
sfn_bbox(sfn_font_t *fontp, const char *text, int *widthp, int *heightp, int *leftp, int *topp)
{
    struct sfn_font *font_state;

    font_state = (struct sfn_font*)fontp;

    return ssfn_bbox(&font_state->ssfn_ctx, text, widthp, heightp, leftp, topp);
}

sfn_font_t  *
sfn_load(const char *file, int size)
{
    void *buf;
    FILE *fp;
    struct stat sb;
    struct sfn_font *font;

    font = calloc(1, sizeof(*font));

    if (stat(file, &sb) != 0) {
        return NULL;
    }

    fp = fopen(file, "r");

    if (!fp) return NULL;

    font = calloc(1, sizeof(*font));
    buf = calloc(1, sb.st_size);

    fread(buf, 1, sb.st_size, fp);
    fclose(fp);

    ssfn_load(&font->ssfn_ctx, buf);
    ssfn_select(&font->ssfn_ctx, SSFN_FAMILY_SANS, NULL, SSFN_STYLE_BOLD, size);

    return (sfn_font_t)font;
}

void 
sfn_render(canvas_t *canvas, int x, int y, sfn_font_t *fontp, const char *text, color_t col)
{
    int i;
    int len;
    ssfn_buf_t buf;
    struct sfn_font *font_state;

    font_state = (struct sfn_font*)fontp;

    buf = (ssfn_buf_t){
        .ptr = (uint8_t*)canvas->pixels->pixels,
        .w = canvas->width,
        .h = canvas->height,
        .p = canvas->width*sizeof(color_t),
        .x = x,
        .y = y,
        .fg = (col | 0xFF000000)
    };

    i = 0;

    do {
        len = ssfn_render(&font_state->ssfn_ctx, &buf, &text[i]);
        i += len;
    } while (len > 0);
}